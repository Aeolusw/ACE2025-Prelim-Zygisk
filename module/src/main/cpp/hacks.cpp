#include "game.hpp"
#include "utils.hpp"
#include <cstring>

// ============================================
// 核心逻辑区域 (Red Zone)
// 这里是你要「填空」练习的地方
// ============================================

// 全局变量保存
uintptr_t g_PlayerController = 0;
uintptr_t g_GName = 0;

// [Interview Trap] FName 的解密算法是各大厂面试常考题
// 这里你需要根据 JS 里的 getName 手写 C++ 版本
std::string getName(uintptr_t objAddr) {
  if (objAddr == 0)
    return "None";
  if (g_GName == 0)
    return "None";

  /* 🛑 YOUR_TASK: Implement FName parsing logic here
     Hint: See solve_aim.js:16
     Need to read fNameId, calculate block/offset, read chunk, read header/len
  */

  // Placeholder return to allow compilation
  return "Unknown";
}

#include "shadowhook_wrapper.hpp"

// Hook Stub 和 原函数指针
static void *g_stub_spawn = nullptr;
// 定义原函数签名: void SpawnProjectile(void* this, void* worldContext, void*
// acc, void* rotation)
// 定义原函数签名: AActor* SpawnProjectile(this, context, acc, rotationPtr,
// arg4)
static void *(*orig_SpawnProjectile)(void *, void *, void *, void *,
                                     void *) = nullptr;

void *HOOK_SpawnProjectile(void *thisPtr, void *worldContext, void *acc,
                           void *rotationPtr, void *arg4) {
  // [Debug]
  LOGD("SpawnProjectile called! this: %p, rot: %p, arg4: %p", thisPtr,
       rotationPtr, arg4);

  // 1. 安全检查: 确保 rotationPtr 不是空指针
  // 2. 逻辑介入: 如果找到了 PlayerController，就读取它的 ControlRotation 并覆盖
  // rotationPtr
  if (rotationPtr != nullptr && g_PlayerController != 0) {
    // 这里的 g_PlayerController 需要在 gameLogicLoop 中通过扫描 Actor 列表获得
    // 为了验证功能，我们先确保如果是值传递闪退，加了检查就不该崩

    // 读取逻辑需要稍后实现，目前先透传，验证签名修复后的稳定性
  }

  // 必须确保原函数被调用，且参数原封不动
  // 并且要接住返回值！
  if (orig_SpawnProjectile) {
    return orig_SpawnProjectile(thisPtr, worldContext, acc, rotationPtr, arg4);
  }
  return nullptr;
}

void hookSpawnProjectile(uintptr_t base) {
  uintptr_t targetAddr = base + Game::SpawnProjectile_Func_Offset;
  LOGI("Hooking SpawnProjectile at %p", (void *)targetAddr);

  // 使用 ShadowHook 进行 Inline Hook
  // API: void *shadowhook_hook_func_addr(void *func_addr, void *new_addr, void
  // **orig_addr);
  g_stub_spawn = shadowhook_hook_func_addr((void *)targetAddr,
                                           (void *)HOOK_SpawnProjectile,
                                           (void **)&orig_SpawnProjectile);

  if (g_stub_spawn == nullptr) {
    int err = shadowhook_get_errno();
    LOGE("Hook SpawnProjectile failed: %d - %s", err,
         shadowhook_to_errmsg(err));
  } else {
    LOGI("Hook SpawnProjectile installed successfully");
  }
}

// 主循环：等待 GWorld 初始化并遍历 Actor
// 对应 JS: onReady / waitForGWorld
void gameLogicLoop(uintptr_t base) {
  // 初始化 GName
  g_GName = base + Game::GName_Offset;

  uintptr_t pGWorld = 0;
  while (true) {
    // 读取 GWorld 指针
    // pGWorld = *(uintptr_t*)(base + Game::GWorld_Offset);
    /* 🛑 YOUR_TASK: Read GWorld safely */

    if (pGWorld != 0) {
      LOGI("GWorld initialized: %p", (void *)pGWorld);
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  // 遍历 Actor (Level -> ActorsArray)
  // See solve_aim.js:65
  /* 🛑 YOUR_TASK: Implement Actor Iteration Loop */
}

// 导出给 main.cpp 调用的入口
void hack_thread() {
  LOGI("============================================");
  LOGI("[ShadowHook_Check] 线程已启动，准备加载模块...");
  LOGI("============================================");

  // 初始化 ShadowHook
  // 改为 SHARED 模式 (0)，兼容性更好
  if (!initShadowHook(0)) {
    LOGE("[!] ShadowHook 初始化失败，无法继续");
    return;
  }

  uintptr_t base = 0;
  while ((base = getModuleBase("libUE4.so")) == 0) {
    LOGD("[*] 正在检索 libUE4.so 基址...");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  LOGI("[*] 成功定位 libUE4.so 基址: %p", (void *)base);

  // 1. 启动 Hook
  hookSpawnProjectile(base);

  // 2. 启动游戏逻辑循环 (找 Actor, 修复 GunOffset)
  gameLogicLoop(base);
}
