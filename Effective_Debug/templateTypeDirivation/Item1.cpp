#include <iostream>
#include <vector>
#include <list>
#include <type_traits>
#include <iterator>

// template<typename T>
// typename std::enable_if<
//     std::is_same<decltype(std::begin(std::declval<T>())), 
//     decltype(std::end(std::declval<T>()))>::value>::type


// func(const T& tmp)
// {
//     for (const auto& i : tmp)
//         std::cout << i << " ";
//     std::cout << std::endl;
// }

template <typename T, std::size_t N>
constexpr std::size_t Cal(T (&)[N]) noexcept
{
    return N; 
}



int main()
{
    // std::vector<int> vec = {1, 2, 3};
    // std::list<char> lst = {'a', 'b', 'c'};

    // func(vec); // 输出: 1 2 3
    // func(lst); // 输出: a b c

    const char name [] = "Charlie";
    std::size_t x = Cal(name);
    std::cout << x << std::endl;
    return 0;
}