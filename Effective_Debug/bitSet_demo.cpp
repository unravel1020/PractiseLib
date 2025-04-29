#include <iostream>
#include <bitset>

int main() {
    std::bitset<8> b("11001010"); // 从字符串初始化
    std::cout << "Initial bitset: " << b << std::endl;

    // 访问特定位置的位
    std::cout << "Bit at position 3: " << b[3] << std::endl;

    // 修改位
    b[3] = 1;
    std::cout << "Modified bitset: " << b << std::endl;

    // 翻转位
    b.flip();
    std::cout << "Flipped bitset: " << b << std::endl;

    std::bitset<8> b1("10101010");
    std::bitset<8> b2("11110000");

    // 位与操作
    std::bitset<8> b_and = b1 & b2;
    std::cout << "Bitwise AND: " << b_and << std::endl;

    // 位或操作
    std::bitset<8> b_or = b1 | b2;
    std::cout << "Bitwise OR: " << b_or << std::endl;

    // 位异或操作
    std::bitset<8> b_xor = b1 ^ b2;
    std::cout << "Bitwise XOR: " << b_xor << std::endl;

    // 位非操作
    std::bitset<8> b_not = ~b1;
    std::cout << "Bitwise NOT: " << b_not << std::endl;

    // 循环遍历bitset中的位
    for (size_t i = 0; i < b.size(); ++i) {
        std::cout << b[i];
    }
    std::cout << std::endl;


    return 0;
}