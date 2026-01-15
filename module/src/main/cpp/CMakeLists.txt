cmake_minimum_required(VERSION 3.22)
project(hellozygisk)

# 1. 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================
# ShadowHook 集成
# [Interview Note] 字节跳动的 Inline Hook 框架
# ============================================
set(SHADOWHOOK_DIR ${PROJECT_ROOT}/external/shadowhook/shadowhook/src/main/cpp)

# 添加 ShadowHook 作为子项目编译
add_subdirectory(${SHADOWHOOK_DIR} ${CMAKE_CURRENT_BINARY_DIR}/shadowhook)

# ============================================
# 主模块
# ============================================
add_library(${PROJECT_NAME} SHARED 
    main.cpp
    hacks.cpp
)

# 2. 核心 Flags
# Zygisk 模块通常需要隐藏符号，只暴露特定的入口点
target_compile_options(${PROJECT_NAME} PRIVATE 
    -fvisibility=hidden
    -fvisibility-inlines-hidden # 同时隐藏内联函数，安全性更高
    -Wall                       # 开启警告
    -O3                         # 优化级别
)


# 5. 包含头文件路径
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SHADOWHOOK_DIR}/include  # ShadowHook 头文件
)

# 6. 链接库
# android: Android 系统库
# log: 安卓日志库
# atomic: 原子操作库 (防止某些架构编译不过)
# shadowhook: 字节跳动 Inline Hook 库
target_link_libraries(${PROJECT_NAME}
    android
    log
    atomic
    shadowhook
)
