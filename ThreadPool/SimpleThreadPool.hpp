//
//  SimpleThreadPool.hpp
//  ThreadPool
//
//  Created by Vagesh Vaibhav on 16/07/17.
//  Copyright © 2017 Vagesh Vaibhav. All rights reserved.
//

#ifndef SimpleThreadPool_hpp
#define SimpleThreadPool_hpp

#include <stdio.h>
#include <thread>
#include <functional>
#include <atomic>
#include "ThreadSafeQueue.hpp"

class ThreadJoiner
{
    std::vector<std::thread> &joinThreads;
public:
    ThreadJoiner(std::vector<std::thread>& threads):joinThreads(threads)
    {
        
    }
    ~ThreadJoiner()
    {
        for(auto& iter : joinThreads)
        {
            iter.join();
        }
    }
};

class SimpleThreadPool
{
    std::atomic<bool> done;
    ThreadSafeQueue<std::function<void()>> taskQueue;
    std::vector<std::thread> myThreads;
    ThreadJoiner joiner;
public:
    SimpleThreadPool():joiner(myThreads),done(false)
    {
        int numberThreads = std::thread::hardware_concurrency();
        try
        {
            for(int iter=0;iter<numberThreads;iter++)
            {
                myThreads.push_back(std::thread(&SimpleThreadPool::WorkerThread,this));
            }
            
        }
        catch(...)
        {
            done = true;
            throw;
        }

        
    }
    ~SimpleThreadPool()
    {
        done = true;
    }
    void WorkerThread()
    {
        while(!done)
        {
            std::function<void()> task;
        
            if(taskQueue.try_pop(task))
            {
                task();
            }
            else
            {
                //Todo
                std::this_thread::yield();
            }
            
        }
        
    }
    
    void Submit(std::function<void()> func)
    {
        taskQueue.push(func);
    }
    
};

#endif /* SimpleThreadPool_hpp */
