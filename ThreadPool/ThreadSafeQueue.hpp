//
//  ThreadSafeQueue.hpp
//  ThreadPool
//
//  Created by Vagesh Vaibhav on 16/07/17.
//  Copyright © 2017 Vagesh Vaibhav. All rights reserved.
//

#ifndef ThreadSafeQueue_hpp
#define ThreadSafeQueue_hpp

#include <stdio.h>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <memory>

template<class T>
class ThreadSafeQueue
{
    std::queue<T> myQueue;
    std::condition_variable condition;
    mutable std::mutex myMutex;
    
public:
    ThreadSafeQueue()
    {
        
    }
    
    ThreadSafeQueue(const ThreadSafeQueue& copy)=delete;
    
    ThreadSafeQueue& operator=(const ThreadSafeQueue& assign)=delete;
    
    void push(const T& data)
    {
        std::unique_lock<std::mutex> lock(myMutex);
        myQueue.push(data);
        condition.notify_one();
    }
    
    std::shared_ptr<T> try_pop()
    {
        std::unique_lock<std::mutex> lock(myMutex);
        if(!myQueue.empty())
        {
            std::shared_ptr<T> ptr = std::make_shared<T>(myQueue.front());
            myQueue.pop();
            return ptr;
        }
        return std::make_shared<T>();
    }
    
    bool try_pop(T& data)
    {
        std::unique_lock<std::mutex>(myMutex);
        if(!myQueue.empty())
        {
            data = myQueue.front();
            myQueue.pop();
            return true;
        }
        return false;
    }
    
    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lock(myMutex);
        condition.wait(lock,[this]()->bool
                       {return !myQueue.empty();});
        std::shared_ptr<T> ptr = std::make_shared<T>(myQueue.front());
        myQueue.pop();
        return ptr;
    }
    
    void wait_and_pop(T& data)
    {
        std::unique_lock<std::mutex> lock(myMutex);
        condition.wait(lock,[this]()->bool
                       {return !myQueue.empty();});
        data = myQueue.front();
        myQueue.pop();
        
    }
    
    bool empty() const
    {
        std::unique_lock<std::mutex> lock(myMutex);
        return myQueue.empty();
    }
    
    
    
};

#endif /* ThreadSafeQueue_hpp */
