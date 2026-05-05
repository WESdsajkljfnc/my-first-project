#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string name;

    if (argc > 1) {
        name = argv[1];   // 取第一个参数（程序名是 argv[0]）
    } else {
        name = "World";   // 默认名字
    }

    std::cout << "Hello, " << name << "!" << std::endl;
    return 0;
}