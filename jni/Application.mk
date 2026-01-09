# Application.mk - 指定目标架构和 API 版本

# 目标 CPU 架构（现代手机基本都是 arm64-v8a）
APP_ABI := arm64-v8a

# 最低支持的 Android API 版本
APP_PLATFORM := android-29

# C++ STL 库
APP_STL := c++_static # c++_static是NDK的宏，链接C++静态库
