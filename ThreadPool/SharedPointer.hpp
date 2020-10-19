//
//  SharedPointer.hpp
//  ThreadPool
//
//  Created by Vagesh Vaibhav on 16/07/17.
//  Copyright © 2017 Vagesh Vaibhav. All rights reserved.
//

#ifndef SharedPointer_hpp
#define SharedPointer_hpp

#include <stdio.h>
#include <iostream>

using std::cout;
using std::endl;

class ReferenceCount
{
    int count;
public:
    ReferenceCount():count(0)
    {
        
    }
    void AddRef()
    {
        ++count;
    }
    int RemoveRef()
    {
        return --count;
    }
    int GetCount()
    {
        return 0;
        //return count;
    }
};

template<class T>
class MySharedPointer
{
    T* ptr;
    ReferenceCount *rc;
    
public:
    MySharedPointer()
    {
        ptr = new T;
        rc = new ReferenceCount;
        rc->AddRef();
    }
    
    MySharedPointer(T* pointer)
    {
        ptr = pointer;
        rc = new ReferenceCount;
        rc->AddRef();
    }
    
    T& operator*()
    {
        return *ptr;
    }
    
    T* operator->()
    {
        return ptr;
    }
    
    MySharedPointer(const MySharedPointer &copy)
    {
        cout<< endl<<"Copy Constructor ref count="<<rc->GetCount();
        ptr = copy.ptr;
        rc = copy.rc;
        rc->AddRef();
    }
    MySharedPointer& operator=(const MySharedPointer &assign)
    {
        cout<<endl<< "Assignment Ref count="<<rc->GetCount();
        if(this != &assign)
        {
            if(rc->RemoveRef() == 0)
            {
                delete ptr;
                delete rc;
            }
            
            ptr = assign.ptr;
            rc = assign.rc;
        }
        return *this;
    }
    ~MySharedPointer()
    {
        cout<<endl<<"Count of the current pointer=";///<<rc->GetCount();
        if(rc->RemoveRef() == 0)
        {
            delete ptr;
            delete rc;
        }
    }
    
};


#endif /* SharedPointer_hpp */
