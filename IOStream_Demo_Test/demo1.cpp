// #include <iostream>
// #include <iomanip>

// int main() {
//     double pi = 3.14159;

//     // 设置输出精度
//     std::cout << std::setprecision(3) << pi << std::endl;

//     // 设置输出宽度和对齐方式
//     std::cout << std::setw(10) << std::left << pi << std::endl;
//     std::cout << std::setw(10) << std::right << pi << std::endl;

//     return 0;
// }
// 3.14
// 3.14      
//       3.14


//  如果不使用控制器设置新的控制标志覆盖的话，精度设置会在当前作用域内持续影响，
//  直到程序结束或是另外创建新的io流对象，如果只有临时的一部分想使用，那么使用临时流对象来设置精度。

#include <iostream>
#include <iomanip>
#include <sstream>

int main() {
    double pi = 3.14159;

    // 创建临时字符串流
    std::ostringstream tempStream;
    tempStream << std::setprecision(3) << pi;
    std::cout << "Temporary precision: " << tempStream.str() << std::endl;

    // 默认精度输出
    std::cout << "Default precision: " << pi << std::endl;

    return 0;
}

// Temporary precision: 3.14
// Default precision: 3.14159