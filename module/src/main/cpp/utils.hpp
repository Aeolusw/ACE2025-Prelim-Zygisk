#pragma once
#include <android/log.h>
#include <chrono>
#include <concepts>
#include <fstream>
#include <link.h>
#include <optional>
#include <span>
#include <string>
#include <sys/uio.h>
#include <thread>
#include <unistd.h>
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

// C++20 Concept: 约束模板只接受 trivially copyable 类型
template <typename T>
concept MemReadable = std::is_trivially_copyable_v<T>;

// ============================================
// Safe Memory Reader (Kernel-Assisted)
// ============================================

// 内部实现：利用 syscall 探测内存有效性
inline bool safeReadBuffer(uintptr_t addr, void *buffer, size_t size) {
  struct iovec local_iov = {buffer, size};
  struct iovec remote_iov = {reinterpret_cast<void *>(addr), size};

  // process_vm_readv 读取自身内存，若 addr 无效返回 -1 而非崩溃
  ssize_t nread = process_vm_readv(getpid(), &local_iov, 1, &remote_iov, 1, 0);

  return nread == static_cast<ssize_t>(size);
}

// 配合 std::optional 的安全读取
template <MemReadable T> inline std::optional<T> tryReadMem(uintptr_t addr) {
  T val{};
  if (safeReadBuffer(addr, &val, sizeof(T))) {
    return val;
  }
  return std::nullopt;
}

// 读取非空指针：地址无效或值为0都返回 nullopt
// 专门用于 GWorld/Actor 这种一定要非空的场景
inline std::optional<uintptr_t> tryReadNonZeroPtr(uintptr_t addr) {
  auto valOpt = tryReadMem<uintptr_t>(addr);
  if (!valOpt || *valOpt == 0) {
    return std::nullopt;
  }
  return valOpt;
}

// 快速版本：仅用于 100% 确定有效的地址（如基址偏移）
template <MemReadable T> inline T readMem(uintptr_t addr) {
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
template <MemReadable T> inline void writeMem(uintptr_t addr, const T &val) {
  memcpy(reinterpret_cast<void *>(addr), &val, sizeof(T));
}

inline void writeFloat(uintptr_t addr, float val) {
  writeMem<float>(addr, val);
}

// ============================================
// Span 遍历辅助 (Actor Array)
// ============================================

inline std::span<uintptr_t> makeActorSpan(uintptr_t arrayAddr, size_t count) {
  return {reinterpret_cast<uintptr_t *>(arrayAddr), count};
}
