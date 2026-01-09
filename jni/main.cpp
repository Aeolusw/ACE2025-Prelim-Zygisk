// ============================================
// Zygisk 模块 Step 1: 寻找 libUE4.so 基址
// 对应 JS 逻辑: main() -> setInterval check libUE4
// ============================================

#include "zygisk.hpp"
#include <android/log.h>
#include <chrono> // std::chrono
#include <cstdlib>
#include <cstring>
#include <fstream> // std::ifstream
#include <link.h>
#include <string> // std::string
#include <thread> // std::thread

// 定义日志 TAG
#define LOG_TAG "Zygisk_UE4_Aim"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// [Interview Trap]
// 为什么不用 dlopen(..., RTLD_NOLOAD)?
// 虽然 dlopen 可以检查库是否加载，但它返回的是 handle (opaque pointer)，
// 而我们需要的是 Linker Map 也就是实际内存 Base Address。
// 读取 /proc/self/maps 是获取模块基址的标准通用方法 (ELF VMA 概念)。
uintptr_t getModuleBase(const char *moduleName) {
  std::ifstream maps("/proc/self/maps");
  if (!maps.is_open())
    return 0;

  std::string line;
  while (std::getline(maps, line)) {
    // maps 格式: 7c00000000-7c00100000 r-xp ... /data/.../libUE4.so
    if (line.find(moduleName) != std::string::npos) {
      size_t dash = line.find('-');
      std::string baseStr = line.substr(0, dash);
      // 16进制字符串转 unsigned long long
      return std::strtoull(baseStr.c_str(), nullptr, 16);
    }
  }
  return 0;
}

// 定义一个结构体，用来传递多个参数
struct CallbackContext {
  const char *targetName; // 要找的模块名
  uintptr_t baseAddr;     // 输出结果
};

// 替换掉 getModuleBase
// 回调函数，每找到一个模块调用一次
int callback(struct dl_phdr_info *info, size_t size, void *data) {
  auto *ctx = static_cast<CallbackContext *>(data);
  // info->dlpi_name 就是模块路径/名字
  if (strstr(info->dlpi_name, ctx->targetName)) {
    // info->dlpi_addr 就是你要的 Base Address！
    ctx->baseAddr = info->dlpi_addr;
    return 1; // 找到了，停止遍历
  }
  return 0; // 继续找下一个
}

uintptr_t getModuleBaseBetter(const char *moduleName) {
  CallbackContext ctx{.targetName = moduleName, // C++20 Designated Initializers
                      .baseAddr = 0};
  dl_iterate_phdr(callback, &ctx);
  return ctx.baseAddr;
}

// 模拟 JS 中的 setInterval 逻辑
void hack_thread() {
  LOGI("[*] 等待 libUE4.so 加载...");

  uintptr_t moduleBase = 0;
  while (true) {
    moduleBase = getModuleBaseBetter("libUE4.so");
    if (moduleBase != 0) {
      break;
    }
    // 对应 JS: setInterval(..., 500)
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  LOGI("[*] libUE4 基址: %p", (void *)moduleBase);

  // TODO: 下一步就是在此处进行 Hook (Dobby / InlineHook)
  // hookSpawnProjectile(moduleBase);
}

class MyModule : public zygisk::ModuleBase {
public:
  void onLoad(zygisk::Api *api, JNIEnv *env) override {
    this->api_ = api;
    this->env_ = env;
    // [诊断日志] 确认模块被加载（每个 App 进程都会打印）
    // LOGI("[onLoad] Zygisk 模块已加载!");
  }

  void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
    // [安全检查] env_ 和 nice_name 可能为空
    if (!env_ || !args->nice_name) {
      LOGI("[pre] env_ 或 nice_name 为空，跳过");
      return;
    }

    const char *packageName = env_->GetStringUTFChars(args->nice_name, nullptr);
    if (!packageName) {
      LOGI("[pre] GetStringUTFChars 失败");
      return;
    }

    // [诊断] 打印每个 App 的包名，帮助调试
    // LOGI("[pre] 当前进程: %s", packageName);

    // [注意] 请将此处替换为你实际游戏的包名
    // 赛题包名: com.ACE2025.Game
    if (strcmp(packageName, "com.ACE2025.Game") == 0) {
      LOGI("[pre] 检测到目标进程: %s", packageName);
      shouldHook_ = true; // 标记需要 Hook，但不在这里启动线程
    }

    env_->ReleaseStringUTFChars(args->nice_name, packageName);
  }

  // [关键修复] 在 postAppSpecialize 中启动 Hook 线程
  // 此时 App 进程已完全初始化，JNI 环境稳定
  void postAppSpecialize(const zygisk::AppSpecializeArgs *args) override {
    if (shouldHook_) {
      LOGI("[post] 启动 hack 线程...");
      // 必须新开线程，否则会阻塞 App 主线程导致 ANR
      std::thread(hack_thread).detach();
    }
  }

private:
  zygisk::Api *api_ = nullptr;
  JNIEnv *env_ = nullptr;
  bool shouldHook_ = false; // 是否需要执行 Hook
};

REGISTER_ZYGISK_MODULE(MyModule)
