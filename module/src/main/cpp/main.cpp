// ============================================
// Zygisk 模块 Step 1: 寻找 libUE4.so 基址
// 对应 JS 逻辑: main() -> setInterval check libUE4
// ============================================

#include "utils.hpp"
#include "zygisk.hpp"

// 声明 hack_thread (在 hacks.cpp 中实现)
extern void hack_thread();

class MyModule : public zygisk::ModuleBase {
public:
  void onLoad(zygisk::Api *api, JNIEnv *env) override {
    this->api_ = api;
    this->env_ = env;
  }

  void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
    if (!args->nice_name)
      return;
    const char *pkg = env_->GetStringUTFChars(args->nice_name, nullptr);
    if (pkg) {
      // [Interview Note] 这种字符串比较效率较低，实际场景可用 Hash 对比
      if (strcmp(pkg, "com.ACE2025.Game") == 0) {
        shouldHook_ = true;
        LOGI("[Zygisk] 锁定目标: %s", pkg);
      }
      env_->ReleaseStringUTFChars(args->nice_name, pkg);
    }
  }

  void postAppSpecialize(const zygisk::AppSpecializeArgs *args) override {
    if (shouldHook_) {
      // [Interview Note] 这里为什么必须 detach?
      // 因为 postAppSpecialize 是在大约 30ms 的主线程时间片内调用的
      // 如果这里阻塞，APP 就会由 Android OS 判定为未响应 (ANR)
      std::thread(hack_thread).detach();
    }
  }

private:
  zygisk::Api *api_ = nullptr;
  JNIEnv *env_ = nullptr;
  bool shouldHook_ = false;
};

REGISTER_ZYGISK_MODULE(MyModule)
