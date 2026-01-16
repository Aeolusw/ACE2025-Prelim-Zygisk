#pragma once
#include <android/log.h>
#include <chrono>
#include <fstream>
#include <link.h>
#include <string>
#include <thread>
#include <vector>

// Start: 基础设施区域 (Green Zone)
// 这里是通用的日志和工具函数，不需要你手写，直接使用即可。

#define LOG_TAG "Zygisk_UE4_Aim"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 回调结构体
struct CallbackContext {
  const char *targetName;
  uintptr_t baseAddr;
};

// 查找模块基址 (dl_iterate_phdr 实现)
// [Interview Note] 这里利用了 ELF Loader 的特性，比单纯 open maps 文件更稳健
inline int callback(struct dl_phdr_info *info, size_t size, void *data) {
  auto *ctx = static_cast<CallbackContext *>(data);
  if (strstr(info->dlpi_name, ctx->targetName)) {
    ctx->baseAddr = info->dlpi_addr;
    return 1;
  }
  return 0;
}

inline uintptr_t getModuleBase(const char *moduleName) {
  CallbackContext ctx{.targetName = moduleName, .baseAddr = 0};
  dl_iterate_phdr(callback, &ctx);
  return ctx.baseAddr;
}

// ============================================
// 内存读取工具 (Memory Read Utilities)
// 模拟 Frida 风格的 API，减少重复 memcpy
// ============================================

// 通用模板：从地址读取任意 POD 类型
template <typename T> inline T readMem(uintptr_t addr) {
  T val{};
  memcpy(&val, reinterpret_cast<void *>(addr), sizeof(T));
  return val;
}

// 语法糖：常用类型的快捷函数
inline uint8_t readU8(uintptr_t addr) { return readMem<uint8_t>(addr); }
inline uint16_t readU16(uintptr_t addr) { return readMem<uint16_t>(addr); }
inline uint32_t readU32(uintptr_t addr) { return readMem<uint32_t>(addr); }
inline uint64_t readU64(uintptr_t addr) { return readMem<uint64_t>(addr); }
inline uintptr_t readPtr(uintptr_t addr) { return readMem<uintptr_t>(addr); }
inline float readFloat(uintptr_t addr) { return readMem<float>(addr); }

// 写入内存
template <typename T> inline void writeMem(uintptr_t addr, const T &val) {
  memcpy(reinterpret_cast<void *>(addr), &val, sizeof(T));
}

inline void writeFloat(uintptr_t addr, float val) {
  writeMem<float>(addr, val);
}
