template <typename T>
struct LowLockMultiProducerQueue
{
private:
	struct Node {
		Node( T* val ) : value(val), next(nullptr) { }
	    T* value;
	    std::atomic<Node*> next;
	    char pad[TT_CACHE_LINE_SIZE - sizeof(T*)- sizeof(std::atomic<Node*>)];
	};

    char pad0[TT_CACHE_LINE_SIZE];

	// for one consumer at a time
	Node* first;

	char pad1[TT_CACHE_LINE_SIZE
	         - sizeof(Node*)];

	// shared among consumers
	std::atomic<bool> consumerLock;

	char pad2[TT_CACHE_LINE_SIZE
	         - sizeof(std::atomic<bool>)];

	// for one producer at a time
	Node* last;

	char pad3[TT_CACHE_LINE_SIZE
	         - sizeof(Node*)];

	// shared among producers
	std::atomic<bool> producerLock;

	char pad4[TT_CACHE_LINE_SIZE
	         - sizeof(std::atomic<bool>)];

public:
	LowLockMultiProducerQueue()
	{
	    first = last = new Node( nullptr );
    	producerLock = consumerLock = false;
  	}
  	~LowLockMultiProducerQueue()
  	{
    	while( first != nullptr )
    	{      // release the list
      		Node* tmp = first;
      		first = tmp->next;
      		delete tmp->value;       // no-op if null
      		delete tmp;
    	}
  	}

  	void Produce( const T& t )
  	{
  		Node* tmp = new Node( new T(t) );
  		while( producerLock.exchange(true) )
    		{ }   // acquire exclusivity
  		last->next = tmp;         // publish to consumers
  		last = tmp;             // swing last forward
  		producerLock = false;       // release exclusivity
	}

	bool Consume( T& result )
	{
		while( consumerLock.exchange(true) )
	    	{ }    // acquire exclusivity

	    Node* theFirst = first;
		Node* theNext = first-> next;
		if( theNext != nullptr ) {   // if queue is nonempty
	  		T* val = theNext->value;    // take it out
	  		theNext->value = nullptr;  // of the Node
	  		first = theNext;          // swing first forward
	  		consumerLock = false;             // release exclusivity

	  		result = *val;    // now copy it back
	  		delete val;       // clean up the value
	  		delete theFirst;      // and the old dummy
	  		return true;      // and report success
		}
		consumerLock = false;   // release exclusivity
    	return false;                  // report queue was empty
	}
};


template<typename T>
class LocklessQueue
{
public:
    LocklessQueue()
        :m_size(0)
    {
    }

    unsigned int Produce(T *p)
    {
        unsigned int prev = m_size.fetch_add(1);
        m_queue.Produce(p);
        return prev+1;
    }

    unsigned int Size()
    {
        return m_size.load();
    }

    bool Consume(T*& p)
    {
        while (m_size.load())
        {
            // since Produce() bumps the size before placing the item on the queue,
            // it's possible the loop runs but Consume() returns false. We loop
            // because it will soon show up
            if (m_queue.Consume(p))
            {
                m_size.fetch_sub(1);
                return true;
            }
        }
        p = nullptr;
        return false;
    }
private:
    miscutils::LowLockMultiProducerQueue<T *> m_queue;
    std::atomic<unsigned int> m_size;
};

class thread_queue
{
public:
    struct Task
    {
        Task() {}
        virtual ~Task(){}
        virtual void Execute() = 0;
        virtual void PostExecute()
        {
            delete this;
        }
    };

    struct FunctorTask : public Task
    {
        FunctorTask(std::function<void()>&& f)
        : m_func(std::forward<std::function<void()>>(f))
        {}
        void Execute() {m_func();}
        std::function<void()> m_func;
    };

    thread_queue()
    : m_bRun(false)
    {
    }

    ~thread_queue()
    {
        stop(false);
    }

    // Start the thread.
    void start()
    {
        if (false == m_bRun)
        {
            m_bRun = true;
            m_thread = std::thread(&thread_queue::thread_main, this);
        }
    }

    // Stop the thread.
    // if bFlush == true, the task will be executed for each item
    // remaining on the queue
    void stop(bool bFlush)
    {
        if (m_bRun == false)
        {
            return;
        }
        m_bRun = false;
        m_cv.notify_one();
        m_thread.join();
        while (m_queue.Size())
        {
            Task *t;
            if (m_queue.Consume(t))
            {
                if (bFlush)
                {
                    t->Execute();
                }
                t->PostExecute();
            }
        }
    }

    // Push back data
    void push_back(Task *t)
    {
        if (!m_bRun)
            return;
        m_queue.Produce(t);
        m_cv.notify_one();
    }

    size_t size()
    {
        return m_queue.Size();
    }
private:
    void thread_main()
    {
        while (m_bRun)
        {
            {
                std::unique_lock<std::mutex> lck(m_mtx);
                m_cv.wait(lck);
                lck.unlock();
                while (m_bRun && m_queue.Size())
                {
                    Task *t;
                    if (m_queue.Consume(t))
                    {
                        t->Execute();
                        t->PostExecute();
                    }
                }
            }
        }
    }
    std::mutex m_mtx;
    std::condition_variable m_cv;
    LocklessQueue<Task> m_queue;
    std::thread m_thread;
    std::atomic<bool> m_bRun;
};
