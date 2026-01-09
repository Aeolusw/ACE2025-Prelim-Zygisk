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
#include <string>  // std::string
#include <thread>  // std::thread


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

// 模拟 JS 中的 setInterval 逻辑
void hack_thread() {
  LOGI("[*] 等待 libUE4.so 加载...");

  uintptr_t moduleBase = 0;
  while (true) {
    moduleBase = getModuleBase("libUE4.so");
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
  void onLoad(zygisk::Api *api, JNIEnv *env) override { this->env_ = env; }

  void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
    const char *packageName = env_->GetStringUTFChars(args->nice_name, nullptr);

    // [注意] 请将此处替换为你实际游戏的包名
    // 赛题包名: com.ACE2025.Game
    if (strcmp(packageName, "com.ACE2025.Game") == 0) {
      LOGI("检测到目标进程: %s", packageName);

      // 必须新开线程，否则会阻塞 App 主线程导致 ANR (Application Not
      // Responding)
      std::thread(hack_thread).detach();
    }

    env_->ReleaseStringUTFChars(args->nice_name, packageName);
  }

private:
  JNIEnv *env_;
};

REGISTER_ZYGISK_MODULE(MyModule)
