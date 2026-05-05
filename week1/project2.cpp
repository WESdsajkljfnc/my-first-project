#include <iostream>
#include <string>

// 引用传参：不拷贝，直接修改外部变量
bool parseArgs(int argc, char* argv[], std::string& name, int& age) {
    if (argc < 3) return false;       // 参数不够
    name = argv[1];
    age = std::stoi(argv[2]);         // 字符串转整数
    return true;
}

int main(int argc, char* argv[]) {
    std::string name;
    int age = 0;

    if (parseArgs(argc, argv, name, age)) {
        std::cout << "Hello, " << name << "! You are " << age << " years old.\n";
    } else {
        std::cout << "Usage: " << argv[0] << " <name> <age>\n";
    }
    return 0;
}
