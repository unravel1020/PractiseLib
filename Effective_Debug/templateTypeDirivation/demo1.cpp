#include <iostream>
#include <type_traits>
#include <utility> // std::declval

// 定义一个类
struct MyClass {
    void sayHello() const { std::cout << "Hello from MyClass!" << std::endl; }
};

// 使用 declval 和 decltype 推导返回类型
template<typename T>
auto getReturnType() -> decltype(std::declval<T>().sayHello()) {
    return std::declval<T>().sayHello();
}

int main() {
    // 使用 decltype 推导类型
    using ReturnType = decltype(getReturnType<MyClass>());
    static_assert(std::is_same<ReturnType, void>::value, "Return type should be void");

    // 检查是否能调用成员函数
    std::cout << "Can call sayHello(): "
              << std::boolalpha
              << std::is_same<decltype(std::declval<MyClass>().sayHello()), void>::value
              << std::endl;

    // 注意：std::declval 不会在运行时生成对象
    // 下面的代码无法编译，因为 declval 不能在运行时使用
    // auto obj = std::declval<MyClass>();
    
    return 0;
}


// #include <iostream>
// #include <type_traits>
// #include <utility> // std::declval

// // 定义一个没有默认构造函数的抽象类
// class ComplexClass {
// public:
//     ComplexClass(int x) : value(x) {} // 无默认构造函数
//     virtual ~ComplexClass() = default;
//     virtual void pureVirtualFunction() = 0; // 纯虚函数
//     virtual double complexFunction() const { return value * 2.0; } // 非纯虚函数

// protected:
//     int value;
// };

// int main() {
//     // 使用 std::declval 伪造一个 ComplexClass 对象
//     using ReturnType = decltype(std::declval<ComplexClass>().complexFunction());

//     // 检查返回类型是否为 double
//     static_assert(std::is_same<ReturnType, double>::value, "Return type should be double");

//     std::cout << "Can call complexFunction(): "
//               << std::boolalpha
//               << std::is_same<decltype(std::declval<ComplexClass>().complexFunction()), double>::value
//               << std::endl;

//     return 0;
// }