# Android.mk - NDK 编译配置文件

LOCAL_PATH := $(call my-dir) # $(call my-dir)是NDK的宏，返回当前目录

include $(CLEAR_VARS) # CLEAR_VARS是NDK的宏，清空所有变量

# 模块名称，编译后会生成 libhellozygisk.so
LOCAL_MODULE := hellozygisk

# 源代码文件
LOCAL_SRC_FILES := main.cpp

# 需要链接的系统库
LOCAL_LDLIBS := -llog # -llog是NDK的宏，链接log库

# C++ 标准
LOCAL_CPPFLAGS := -std=c++17

# 编译为共享库
include $(BUILD_SHARED_LIBRARY)
# 共享库是动态链接库，可以被其他模块调用