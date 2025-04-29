#include <iostream>
#include <string>

int main() {
    int num;
    std::cout << "Enter a number: ";
    std::cin >> num;

    std::string fullName;
    std::cout << "Enter your full name: ";
    std::getline(std::cin, fullName);
    std::cout << "Hello, " << fullName << "!" << std::endl;

    // 检查输入操作是否成功
    if (std::cin.fail()) {
        std::cerr << "Invalid input!" << std::endl;
    } else {
        std::cout << "You entered: " << num << std::endl;
    }

    return 0;
}


