#include <iostream>
#include <utility> // std::forward

// 目标函数：区分左值和右值
void process(int& value) {
    std::cout << "Processing Lvalue: " << value << std::endl;
}

void process(int&& value) {
    std::cout << "Processing Rvalue: " << value << std::endl;
}

// 转发函数模板
template <typename T>
void forwarder(T&& param) {
    process(std::forward<T>(param)); // 完美转发
}

int main() {
    int x = 42;

    forwarder(x);       // 左值
    forwarder(42);      // 右值

    return 0;
}