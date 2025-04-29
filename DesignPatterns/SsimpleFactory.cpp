#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <functional>

// 基类 Shape
class Shape {
public:
    virtual void draw() = 0; // 抽象方法
    virtual ~Shape() = default;
};

// 具体类 Circle
class Circle : public Shape {
public:
    void draw() override {
        std::cout << "Drawing a Circle" << std::endl;
    }
};

// 具体类 Rectangle
class Rectangle : public Shape {
public:
    void draw() override {
        std::cout << "Drawing a Rectangle" << std::endl;
    }
};

// 工具函数：将字符串转换为小写并去除前后空格
std::string normalizeString(const std::string& str) {
    std::string result = str;
    // 去除前后空格
    result.erase(0, result.find_first_not_of(" \t\n\r"));
    result.erase(result.find_last_not_of(" \t\n\r") + 1);
    // 转换为小写
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

// 简单工厂类
class ShapeFactory {
private:
    static std::unordered_map<std::string, std::function<std::unique_ptr<Shape>()>> registry;

public:
    static void registerShape(const std::string& shapeType, std::function<std::unique_ptr<Shape>()> creator) {
        registry[normalizeString(shapeType)] = creator;
    }

    static std::unique_ptr<Shape> createShape(const std::string& shapeType) {
        auto it = registry.find(normalizeString(shapeType));
        if (it != registry.end()) {
            return it->second();
        }
        throw std::invalid_argument("Unknown shape type: " + shapeType);
    }
};

// 初始化静态成员变量
std::unordered_map<std::string, std::function<std::unique_ptr<Shape>()>> ShapeFactory::registry;

// 注册形状类型
static void initializeRegistry() {
    ShapeFactory::registerShape("circle", [] { return std::make_unique<Circle>(); });
    ShapeFactory::registerShape("rectangle", [] { return std::make_unique<Rectangle>(); });
}

int main() {
    // 初始化注册表
    static bool initialized = false;
    if (!initialized) {
        initializeRegistry();
        initialized = true;
    }

    try {
        auto circle = ShapeFactory::createShape("circle");
        auto rectangle = ShapeFactory::createShape("rectangle");

        circle->draw();
        rectangle->draw();

        // 测试边界条件
        auto unknownShape = ShapeFactory::createShape("triangle"); // 应抛出异常
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}