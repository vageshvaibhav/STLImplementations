//
//  main.cpp
//  ThreadPool
//
//  Created by Vagesh Vaibhav on 11/06/17.
//  Copyright © 2017 Vagesh Vaibhav. All rights reserved.
//

#include <iostream>
#include<stdio.h>
#include"SharedPointer.hpp"
#include"ThreadSafeQueue.hpp"

int main(int argc, const char * argv[]) {
    MySharedPointer<int> temp(new int(5));
    std::cout<<*temp;
    
    MySharedPointer<int> temp2=temp;
    
    ThreadSafeQueue<int> myQueue;
    
    
//    int ram = 0x01234567;
//    int n =  sizeof(ram);
//    for (int i = 0; i < n; i++)
//    {
//        printf(" %.2x", ((char*)(&ram))[i]);
//        printf("\n");
//    }
}


