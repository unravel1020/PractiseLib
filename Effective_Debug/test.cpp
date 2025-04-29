#include <iostream>
#include <list>

int main() {
    std::list<int> lst1 = {1, 3, 5, 7};
    std::list<int> lst2 = {2, 4, 6, 8};

    lst1.merge(lst2);              // 合并两个已排序的链表,合并后仍是有序的

    std::cout << "Merged and reversed list: ";
    for (const auto& elem : lst1) {
        std::cout << elem << " ";
    }
    std::cout << std::endl;

    lst1.reverse();                // 反转链表

    // 输出链表内容
    std::cout << "Merged and reversed list: ";
    for (const auto& elem : lst1) {
        std::cout << elem << " ";
    }
    std::cout << std::endl;

    return 0;
}

//constexpr 在C++11中只能获取和返回字面值，只能应用与单行函数语句，在C++14中限制相对宽松，可以接受多行语句，不只是字面值
//