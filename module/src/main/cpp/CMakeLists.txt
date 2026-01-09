cmake_minimum_required(VERSION 3.10)
project(hellozygisk)

# 1. 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. 核心 Flags
# Zygisk 模块通常需要隐藏符号，只暴露特定的入口点
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")

# 3. 引入 Dobby 子项目
# 这一步会自动处理 Dobby 的编译，生成 libdobby.a
# 确保在 clone 时使用了 recursive 或者后续执行了 git submodule update --init
# add_subdirectory(third_party/dobby)
# target_include_directories(hellozygisk PRIVATE third_party/dobby/include)
# dobby

# 4. 添加源文件
add_library(
        hellozygisk
        SHARED
        main.cpp
)

# 5. 包含头文件路径
target_include_directories(hellozygisk PRIVATE third_party/dobby/include)
target_include_directories(hellozygisk PRIVATE .)

# 6. 链接库
# android: Android 系统库
# log: 安卓日志库
# atomic: 原子操作库 (防止某些架构编译不过)
# dobby: Dobby Inline Hook 库
target_link_libraries(
        hellozygisk
        android
        log
        atomic
        # dobby
)
