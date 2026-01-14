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
