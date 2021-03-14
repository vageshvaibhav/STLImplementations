#include <map>
#include <set>
#include <list>
#include <cmath>
#include <ctime>
#include <deque>
#include <queue>
#include <stack>
#include <string>
#include <bitset>
#include <cstdio>
#include <limits>
#include <vector>
#include <climits>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <numeric>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>

/*
//A number should be prime if it has only two divisors, self and 1 e.g. 2 , numOfDiv = 2,1

template<int first,int second>
struct IsDivisible
{
    static constexpr int value = (first % second == 0) ? 1 : 0;
};

//Handle division by 0 as base case
template <int first>
struct IsDivisible<first,0>
{
    static constexpr int value = -1;
};

template <int begin, int end>
struct NumberDivisorHelper
{
    static constexpr int value = IsDivisible<end,begin>::value + NumberDivisorHelper<begin+1,end>::value;
};

//termination case
template <int end>
struct NumberDivisorHelper<end,end>
{
    static constexpr int value = 1;
};

template<int A>
struct NumberOfDivisors
{
    static constexpr int value = NumberDivisorHelper<1,A>::value;
};
*/



template<int A>
struct is_prime
{
    template <int temp, bool end>
    struct prime_helper
    {
        static constexpr bool verify_end = (temp*temp > A);
        static constexpr bool is_divisible = (A % temp == 0);
        static constexpr bool negative = A < 0;
        static constexpr bool value = (!negative) ? (verify_end ? true : (is_divisible ? false : prime_helper<temp+1,verify_end || is_divisible>::value)) : false;
    };
    
    template <int temp>
    struct prime_helper<temp,true>
    {
        static constexpr bool value = false;
    };
    
    static constexpr bool value = prime_helper<2,false>::value; 
};

//0 and 1 are base cases
template<>
struct is_prime<0>
{
    static constexpr bool value = false;
};

template<>
struct is_prime<1>
{
    static constexpr bool value = false;
};

template<typename T, T... Ints>
constexpr T get(std::integer_sequence<T, Ints...>, std::size_t i)
{
    constexpr T arr[] = {Ints...};
    return arr[i];
}

template<typename A,int i,int j>
struct FindHelper
{
    static constexpr int value = ((is_prime<get(A{},i)>::value)) ? i : (FindHelper<A,i+1,j>::value);
};

template<typename A,int i>
struct FindHelper<A,i,i>
{
    static constexpr int value = (i < A{}.size()) ? (is_prime<get(A{},i)>::value ? i : -1) : -1;
};

template<typename A>
struct find
{
    static constexpr int value = FindHelper<A,0,A{}.size() - 1>::value;  
};

int main() {

    std::cout << is_prime<0>::value << std::endl;
    std::cout << is_prime<1>::value << std::endl;
    std::cout << is_prime<2>::value << std::endl;
    std::cout << is_prime<3>::value << std::endl;
    
    std::cout << find< std::integer_sequence<int, 0> >::value << std::endl;
    std::cout << find< std::integer_sequence<int, 1> >::value << std::endl;
    std::cout << find< std::integer_sequence<int, 2> >::value << std::endl;
    std::cout << find< std::integer_sequence<int, 3> >::value << std::endl;
    std::cout << find< std::integer_sequence<int, 4> >::value << std::endl;
    std::cout << find< std::integer_sequence<int, 5> >::value << std::endl;
    std::cout << find< std::integer_sequence<int, 6> >::value << std::endl;
    
    std::cout << find< std::integer_sequence<int, 0, 1> >::value << std::endl;
    std::cout << find< std::integer_sequence<int, 0, 1, 2> >::value << std::endl;
    std::cout << find< std::integer_sequence<int, 0, 1, 2, 3> >::value << std::endl;
    std::cout << find< std::integer_sequence<int, 0, 1, 2, 3, 4> >::value << std::endl;
    std::cout << find< std::integer_sequence<int, 0, 1, 2, 3, 4, 5> >::value << std::endl;
    std::cout << find< std::integer_sequence<int, 0, 1, 2, 3, 4, 5, 6> >::value << std::endl;
    
    std::cout << find< std::integer_sequence<int, 4, 8, 15, 16, 23, 42> >::value << std::endl;
    
    std::cout << find< std::integer_sequence<int, 198, 4, 667, 917, 74, 468, 817, 705, 908, 517, 88, 353, 975, 903, 326, 25, 170, 629, 992, 724, 587, 411, 536, 250, 268, 727, 643, 547, 181> >::value << std::endl;
    std::cout << find< std::integer_sequence<int, 559, 917, 917, 198, 670, 522, 184, 521, 364, 227, 6, 294, 295> >::value << std::endl;
    std::cout << find< std::integer_sequence<int, 590, 765, 522, 825, 722, 927, 646, 422, 453, 186, 673, 897, 690, 408, 217, 841, 259, 274, 254, 523, 758, 325, 350, 353, 561, 384, 40, 981, 623, 35, 0, 461, 446, 0, 394> >::value << std::endl;
    std::cout << find< std::integer_sequence<int, 590, 765, 522, 825, 722, 927, 646, 422, 453, -2, 673, 897, 690, 408, 217, 841, 259, 274, 254, 523, 758, 325, 350, 353, 561, 384, 40, 981, 623, 35, 0, 461, 446, 0, 394> >::value << std::endl;   
    
    return 0;
}
