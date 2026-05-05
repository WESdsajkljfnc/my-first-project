# 仓库简介
## 这是一个记录培训任务完成情况的仓库
## 主要目录分为：
1.仓库简介
2.周度记录
3.项目说明

# 周度记录
## W1
- 本周做了什么
  1. 本周搭建了一个C++工程环境，把项目文件变成了一个本地仓库
  2. 在电脑上安装了编译器（GCC/Clang）、CMake、Git 和编辑器（VS Code）
  3. 创建了一个项目文件夹（cpp_week1），并在里面新建了 main.cpp 文件
  4. 不借助CMake，直接用编译器把源代码变成可执行程序
  5. 引入CMake构建系统，当代码很多时用3.的方法就太麻烦了，所以要用Cmake
  6. C++进度：学了函数，引用，简单类，vector/string/map，对于头文件和源文件拆分不太熟悉
  
- 提交的代码：
  1. 一个输入名字，输出问好的C++代码  ![运行截图](images/1.png)
  2. 将原来的问候代码进行改进，改为读取命令行参数的问候代码![运行截图](images/4.png)
  3. project1
  4. project2
  
- 遇到的问题：
  1. 编译时找不到文件
  2. 不理解为什么不直接把工作空间（包含源码，README.md）分享给别人，非要用git
  3. camke报错![报错情况](images/3.png)

 - 如何解决的：
  1. 原来没有把buid建在工作空间根目录，所以找不到文件
  2. 问AI，是因为git可以记录每一次commit的快照，随时可以查看原来的版本
  3. 发现是由于我把工作空间的名字改为了Week1，而缓存文件在旧的data/build目录下，现在是Week1/build，   路径不匹配,在build目录下终端输入rm -rf CMakeCache.txt CMakeFiles,把旧文件删掉就行了

 
- 还没有解决：
  1. cmake如何编译多个.cpp文件
  2. 建立src文件夹
  
## W2
 
## W3  
  
# 项目说明
## W1
- 项目功能：问候
- 编译方式：最小Cmake:
```cmake
cmake_minimum_required(VERSION 3.10)
project(Hello)
add_executable(hello main.cpp)
```
第一句：设置 CMake 的最低版本要求（3.10）
第二句：定义项目名
第三句：告诉Cmake用main.cpp文件生成一个叫hello的可执行文件

