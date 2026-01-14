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

// Hook 回调函数
// 目标: 修改 SpawnProjectile 的参数，让子弹射向准星方向
void HOOK_SpawnProjectile(void *thisPtr, void *worldContext, void *acc,
                          void *rotationPtr) {
  LOGD("SpawnProjectile called!");

  /* 🛑 YOUR_TASK: Implement the Aimbot Logic
     1. Check if g_PlayerController is valid.
     2. Read ControlRotation from PlayerController (Offset 0x288).
     3. Overwrite *rotationPtr with the read Pitch/Yaw.
  */
}

void hookSpawnProjectile(uintptr_t base) {
  uintptr_t targetAddr = base + Game::SpawnProjectile_Func_Offset;
  LOGI("Hooking SpawnProjectile at %p", (void *)targetAddr);

  // TODO: 使用 Dobby 进行 Inline Hook
  // DobbyHook((void*)targetAddr, (void*)HOOK_SpawnProjectile,
  // (void**)&orig_SpawnProjectile);
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
  LOGI("[*] 线程启动，等待 libUE4.so...");

  uintptr_t base = 0;
  while ((base = getModuleBase("libUE4.so")) == 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  LOGI("[*] 找到 libUE4.so 基址: %p", (void *)base);

  // 1. 启动 Hook
  hookSpawnProjectile(base);

  // 2. 启动游戏逻辑循环 (找 Actor, 修复 GunOffset)
  gameLogicLoop(base);
}
