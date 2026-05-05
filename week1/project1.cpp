#include <iostream>
#include <string>

class Greeter {
private:
    std::string name;
    int age;
public:
    // 构造函数 + 初始化列表
    Greeter(const std::string& n, int a) : name(n), age(a) {}

    void sayHello() const {
        std::cout << "Hello, " << name
                  << " (age " << age << ")!" << std::endl;
    }
};

int main() {
    Greeter g("Alice", 20);
    g.sayHello();
    return 0;
}
