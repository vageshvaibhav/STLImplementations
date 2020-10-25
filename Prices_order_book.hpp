//https://quant.stackexchange.com/questions/3783/what-is-an-efficient-data-structure-to-model-order-book
// Few points around Order book implemetation to consider
/*

The specifics depend on if you're implementing for equities (order-based) or futures (level-based). I recommend https://web.archive.org/web/20110219163448/http://howtohft.wordpress.com/2011/02/15/how-to-build-a-fast-limit-order-book/ for a general overview of a good architecture for the former.

Building off of that, though, I have found that using array-based over pointer-based data structures provides faster performance. This is because for modern pipelining, caching CPUs, branching and dereferences (especially from uncached memory) are the biggest performance killers because they introduce data dependencies resulting in pipeline / memory stalls.

If you can, you should also optimize for the particular exchange. For instance, it turns out that, last I checked, Nasdaq generates order IDs incrementally starting from a small number, so you can store all the orders in a giant array instead of a hashtable. This is really cache- and TLB-friendly compared to a hashtable because most updates tend to happen to recently-dereferenced orders.

Speaking from experience with equities (order-based), my preferred architecture is:

A large associative array from order ids to order metadata (std::unordered_map or std::vector if you can swing it such as in the case of Nasdaq).
The order metadata includes pointers to the order book (essentially consisting of the price-levels on both sides) and price-level it belongs to, so after looking up the order, the order book and price level data structures are a single dereference away. Having a pointer to the price allows for a O(1) decrement for an Order Execute or Order Reduce operation. If you want to keep track of time-priority as well, you can keep pointers to the next and previous orders in the queue.
Since most updates happen near the inside of the book, using a vector for the price levels for each book will result in the fastest average price lookup. Searching linearly from the end of the vector is on average faster than a binary search, too, because most of the time the desired price is only a few levels at most from the inside, and a linear search is easier on the branch predictor, optimizer and cache. Of course, there can be pathological orders far away from the inside of the book, and an attacker could conceivably send a lot of updates at the end of the book in order to slow your implementation down. In practice, though, most of the time this results in a cache-friendly, nearly O(1) implementation for insert, lookup, update and delete (with a worst-case O(N) memcpy). Specifically for a Best Bid / Offer (BBO) update, you can get an implementation to calculate the update in just a few instructions (essentially appending or deleting an element from the end of the vector), or about three dereferences.
The behavior of this is O(1) best-case behavior for insertion, lookup, delete and update with very low constants (I've clocked it to be on average the cost of a single fetch from main memory - roughly 60ns). Unfortunately you can get O(N) worst case behavior, but with low probability and still with very good constants due to the cache-, TLB- and compiler-friendliness. It is also very fast, nearly optimally so, in the case of BBO updates which is what you are usually interested in anyways.
I have a reference implementation here for Nasdaq's ITCH protocol which illustrates these techniques and more. It clocks in at a mean of just over 61ns / tick (about 16 million ticks / second) by only keeping track of the quantity at each price-level and not the order queue, and uses almost no allocation besides std::vector resizing.
https://github.com/charles-cooper/itch-order-book
*/



enum eSide { eBids = 0, eAsks = 1 };

inline eSide OtherSide (eSide side) { return side == eBids ? eAsks : eBids; }

struct sPriceLevel
{
    mds_ns::Price_t price;
    mds_ns::Quantity_t qty;
    uint32_t numOrders;
};

typedef uint64_t OrderId_t;
typedef uint64_t Priority_t;
struct sOrder
{
    OrderId_t orderId;
    mds_ns::Price_t price;
    mds_ns::Quantity_t quantity;
    Priority_t priority;
};

// Single price level of orders sorted by order priority.  Based on the assumption that we'll spend
// more time reserializing this collection to republish snapshots than we will actually finding
// orders by ID to change/delete them.  The time cost on the latter should be minimized because we
// key the top-level collection by price.
typedef std::map<Priority_t, sOrder> OrdersByPriority_t;

// The aforementioned top-level collection of all orders for a given book side.
typedef std::map<mds_ns::Price_t, OrdersByPriority_t> OrdersMap_t;


//--- Components for detailed order book ---
struct OrderatAggregateLevel
{
    OrderId_t orderId;
    mds_ns::Quantity_t quantity;
    mds_ns::Priority_t priority;
};

struct AggregateData
{
    mds_ns::Quantity_t cumQty;
    std::vector<OrderatAggregateLevel> orders;
};

typedef std::map<int64_t, AggregateData, std::function<bool(int64_t, int64_t)>> Aggregate_t;

struct OrderData
{
    mds_ns::Price_t price;
    mds_ns::Quantity_t quantity;
    Priority_t priority;
    Aggregate_t::iterator aggBookIter;
};
typedef std::unordered_map<uint64_t, OrderData> Orders_t;

// --- detailed order book ---

inline size_t GetTotalOrders(const OrdersMap_t& omap)
{
    size_t out = 0;
    for (const auto& it : omap)
        out += it.second.size();
    return out;
}

struct OrderPublishLimits {
    size_t numLevels;
    size_t numOrders;
};

inline size_t GetLevelsCount(const OrdersMap_t& omap, const size_t maxOrders)
{
    size_t out = 0;
    size_t counter = maxOrders;
    auto levelIt = omap.cbegin();
    while (counter > 0)
    {
        // One more order level.
        ++out;

        // Figure out how many remaining orders we can add for this price level.
        // If this level has the last of the orders we can publish, we're done processing.
        size_t levelOrders = std::min(counter, levelIt->second.size());
        counter -= levelOrders;
        if (counter == 0)
            break;

        ++levelIt;
    }

    return out;
}

inline size_t GetLevelsCount(const Aggregate_t& omap, const size_t maxOrders)
{
    size_t out = 0;
    size_t counter = maxOrders;
    auto levelIt = omap.cbegin();
    while (counter > 0)
    {
        // One more order level.
        ++out;

        // Figure out how many remaining orders we can add for this price level.
        // If this level has the last of the orders we can publish, we're done processing.
        size_t levelOrders = std::min(counter, levelIt->second.orders.size());
        counter -= levelOrders;
        if (counter == 0)
            break;

        ++levelIt;
    }

    return out;
}

inline std::ostream& operator<<(std::ostream& os, const OrderData& order)
{
    os << "price=" << order.price
        << " quantity=" << order.quantity
        << " priority=" << order.priority;
    return os;

}



class IEventBook
{
public:
    virtual void OnEvent(const MarketEventPtr event) = 0;
    virtual void Clear() = 0;
    virtual void ClearPriceStat(const ps_messages_ns::StatPriceID::Value sp) = 0;

    virtual void EncodeBookInSBE(ps_messages_ns::SnapshotMsg_v2 &msg, const uint32_t maxLevelsToEncode,
                                const int64_t priceDivisor, const uint16_t qtyFactor) = 0;
    virtual void EncodeOrdersInSBE(ps_messages_ns::SnapshotMsg_v2 &msg, const uint32_t maxLevelsToEncode,
                                const int64_t priceDivisor, const uint16_t qtyFactor) = 0;
    virtual void EncodeImpliedBookInSBE(ps_messages_ns::SnapshotMsg_v2 &msg, const uint32_t levelsInterest,
                                const int64_t priceDivisor, const uint16_t qtyFactor) = 0;

    virtual void EncodeOrderIDsInSBE(ps_messages_ns::SnapshotMsg_v2& msg, const uint32_t idsInterest)
    {
        msg.bidOrderIDsCount(0);
        msg.bidOrderIDsCount(0);
    }

    virtual void EncodeCounterPartyIDsInSBE(ps_messages_ns::SnapshotMsg_v2& msg, const uint32_t idsInterest)
    {
        msg.bidCounterPartyIDsCount(0);
        msg.askCounterPartyIDsCount(0);
    }

    virtual ps_messages_ns::HitOrTake::Value CalculateHitOrTake(const mds_ns::Price_t lastTradedPrice) = 0;

    virtual bool IsCrossed() = 0;

    virtual void EncodeBookInSBE(ps_messages_ns::SnapshotMsg_v3& msg, const uint32_t maxLevelsToEncode,
                            const int64_t priceDivisor, const uint16_t qtyFactor)
    {
        msg.bidsCount(0);
        msg.asksCount(0);
    }

    virtual void EncodeOrdersInSBE(ps_messages_ns::SnapshotMsg_v3& msg, const uint32_t maxLevelsToEncode,
                                const int64_t priceDivisor, const uint16_t qtyFactor)
    {
        msg.bidOrdersCount(0);
        msg.askOrdersCount(0);
    }

    virtual void EncodeImpliedBookInSBE(ps_messages_ns::SnapshotMsg_v3& msg, const uint32_t levelsInterest,
                                    const int64_t priceDivisor, const uint16_t qtyFactor)
    {
        msg.impliedBidsCount(0);
        msg.impliedAsksCount(0);
    }

    virtual void EncodeOrderIDsInSBE(ps_messages_ns::SnapshotMsg_v3& msg, const uint32_t idsInterest)
    {
        msg.bidOrderIDsCount(0);
        msg.bidOrderIDsCount(0);
    }

    virtual void EncodeCounterPartyIDsInSBE(ps_messages_ns::SnapshotMsg_v3& msg, const uint32_t idsInterest)
    {
        msg.bidCounterPartyIDsCount(0);
        msg.askCounterPartyIDsCount(0);
    }

    virtual std::string ToString() { return "unsupported"; }

    virtual const Orders_t& GetOrderBook(const eSide side) const
    {
        static const Orders_t s_empty {};
        return s_empty;
    }
};

typedef std::unique_ptr<IEventBook> EventBookPtrT;



class OrderBook : public IEventBook
{
public:
    virtual void OnEvent(const MarketEventPtr event) override;
    virtual void Clear() override;
    virtual void ClearPriceStat(const ps_messages_ns::StatPriceID::Value sp) override;

    virtual void EncodeBookInSBE(ps_messages_ns::SnapshotMsg_v2& msg, const uint32_t levelsInterest,
            const int64_t priceDivisor, const uint16_t qtyFactor) override;
    virtual void EncodeOrdersInSBE(ps_messages_ns::SnapshotMsg_v2& msg, const uint32_t levelsInterest,
            const int64_t priceDivisor, const uint16_t qtyFactor) override;
    virtual void EncodeImpliedBookInSBE(ps_messages_ns::SnapshotMsg_v2& msg, const uint32_t levelsInterest,
            const int64_t priceDivisor, const uint16_t qtyFactor) override;
    virtual void EncodeOrderIDsInSBE(ps_messages_ns::SnapshotMsg_v2& msg, const uint32_t ordersInterest) override;

    virtual bool IsCrossed() override;

    virtual std::string ToString() override;

    ps_messages_ns::HitOrTake::Value CalculateHitOrTake(const mds_ns::Price_t lastTradedPrice) override;
    virtual const Orders_t& GetOrderBook(const eSide side) const;

    struct sBookSide
    {
        sBookSide(eSide side);

        void AddOrder(const uint64_t orderId, const int64_t orderPrice, const int64_t aggregatePrice,
                const int64_t quantity, const uint64_t priority);
        void ChangeOrder(const uint64_t orderId, const int64_t orderPrice, const int64_t aggregatePrice,
                const int64_t quantity, const uint64_t priority);
        void AddOrChangeOrder(const uint64_t orderId, const int64_t orderPrice, const int64_t aggregatePrice,
                const int64_t quantity, const uint64_t priority);
        void ExecuteOrder(const uint64_t orderId, const int64_t quantity,  uint64_t priority);
        void DeleteOrder(const uint64_t orderId);

        void RollupBookSide(Price_t eqPrice);
        void UnrollBookSide();
        void RemoveOrderFromAggregatePrice(Orders_t::iterator& orderIter);
        void AddOrderToAggregatePrice(Orders_t::iterator& orderIter, Price_t aggrPrice);

        size_t Size() const;
        mds_ns::Price_t InsidePrice() const;

        void Clear();

        const eSide m_side;

        Orders_t m_orderBook;
        Aggregate_t m_aggregateBook;
    };

    const sBookSide& Bids() const;
    const sBookSide& Asks() const;

    const Orders_t& BidOrders() const;
    const Orders_t& AskOrders() const;
    bool HasOrders() const;

    Price_t GetIndicativeOpenPrice() const;

    OrderBook()
        : m_book({
            (sBookSide(eBids)),
            (sBookSide(eAsks))
        }) {}

private:
    void OnOrderBook(const MarketEventPtr event);
    void OnStatPrice(const MarketEventPtr event);
    void RollupAggregateBookIfNeeded();
    void UnrollAggregateBook();
    bool ShouldAggregateBookRollUp() const;
    mds_ns::Price_t GetAggregatePrice(mds_ns::Price_t orderPrice, eSide side) const;

    std::array<sBookSide, 2> m_book;
    Price_t m_indicativeOpenPrice = ADAPTER_INVALID_INT;
};
