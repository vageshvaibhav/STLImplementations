//
//  MySingleton.hpp
//  ThreadPool
//
//  Created by Vagesh Vaibhav on 17/07/17.
//  Copyright © 2017 Vagesh Vaibhav. All rights reserved.
//

#ifndef MySingleton_hpp
#define MySingleton_hpp

#include <stdio.h>
#include <mutex>
#include <memory>


class MySIngleton
{
    static std::unique_ptr<MySIngleton> instance;
    static std::once_flag flag;
    MySIngleton()=default;
    MySIngleton(const MySIngleton&)=delete;
    MySIngleton& operator=(const MySIngleton&)=delete;

public:
    ~MySIngleton()
    {
        
    }
    static MySIngleton& GetInstance()
    {
        std::call_once(flag,[](){instance.reset(new MySIngleton);});
        return *instance;
    }
};

std::unique_ptr<MySIngleton> MySIngleton::instance = nullptr;
std::once_flag MySIngleton::flag;


#endif /* MySingleton_hpp */
