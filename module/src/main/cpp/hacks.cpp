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

  /* 🛑 填空题: 实现 FName 解析逻辑
     提示: 参考 solve_aim.js:16-32
     步骤:
     1. 从 objAddr + 0x18 读取 fNameId
     2. 计算 block = fNameId >> 16, offset = fNameId & 0xFFFF
     3. 从 GName + 0x30 获取 fNamePool
     4. 读取 chunk = fNamePool + 0x10 + block * 8
     5. 计算 entry = chunk + 2 * offset
     6. 读取 header 并解析 len = header >> 6
     7. 从 entry + 2 读取 len 个字符作为名称
  */
  auto fNameId = readU32(objAddr + 0x18);
  auto block = fNameId >> 16;
  auto offset = fNameId & 0xFFFF;
  auto fNamePool = g_GName + 0x30;

  auto chunk = readPtr(fNamePool + 0x10 + block * 8);
  auto entry = chunk + 2 * offset;

  auto header = readU16(entry);
  auto len = header >> 6;
  if (len > 0 && len < 100) {
    // String is at entry + 2
    return std::string((char *)(entry + 2), len);
  }

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
static void *(*orig_SpawnProjectile)(void *, void *, void *, void *, void *,
                                     void *) = nullptr;
void *HOOK_SpawnProjectile(void *thisPtr, void *arg1, void *arg2, void *arg3,
                           void *arg4, void *arg5) {
  // arg3 = rotationPtr (UE4: FRotator*)
  void *rotationPtr = arg3;

  // Aimbot 逻辑: 只有找到 PlayerController 且 rotationPtr 有效时才执行
  if (g_PlayerController != 0 && rotationPtr != nullptr) {
    // 读取玩家当前视角 (ControlRotation)
    auto ctrlRotPtr = g_PlayerController + 0x288;
    auto pitch = readFloat(ctrlRotPtr);
    auto yaw = readFloat(ctrlRotPtr + 4);

    // 强行修正子弹发射角度 (Aimbot)
    writeFloat((uintptr_t)rotationPtr, pitch);
    writeFloat((uintptr_t)rotationPtr + 4, yaw);
  }

  // 调用原函数
  if (orig_SpawnProjectile) {
    return orig_SpawnProjectile(thisPtr, arg1, arg2, arg3, arg4, arg5);
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
  // 轮询等待游戏世界对象初始化，避免访问空指针
  while (true) {
    // 读取 GWorld 指针
    // pGWorld = *(uintptr_t*)(base + Game::GWorld_Offset);
    /* 🛑 YOUR_TASK: Read GWorld safely */
    pGWorld = readPtr(base + Game::GWorld_Offset);

    if (pGWorld != 0) {
      LOGI("GWorld initialized: %p", (void *)pGWorld);
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  // 遍历 Actor (Level -> ActorsArray)
  // See solve_aim.js:65-95
  /* 🛑 填空题: 实现 Actor 遍历循环
     提示: 参考 solve_aim.js:65-95
     步骤:
     1. 从 pGWorld + 0x30 读取 Level 指针
     2. 从 Level + 0x98 读取 ActorsArray 指针
     3. 从 Level + 0x98 + 8 读取 ActorsCount
     4. 循环遍历: actor = actorsArray + i * 8
     5. 对每个 actor 调用 getName() 获取名称
     6. 如果是 "FirstPersonCharacter" -> 修复 GunOffset（见下方填空）
     7. 如果是 "PlayerController" -> 保存到 g_PlayerController
  */
  auto level = readPtr(pGWorld + 0x30);
  auto actorsArray = readPtr(level + 0x98);
  auto actorsCount = readU32(level + 0x98 + 8);

  for (uint32_t i = 0; i < actorsCount; i++) {
    auto actor = readPtr(actorsArray + i * 8);
    if (actor == 0)
      continue;

    auto name = getName(actor);

    // 1. 找到 FirstPersonCharacter -> 修复 GunOffset
    if (name.find("FirstPersonCharacter") != std::string::npos) {
      LOGI("[!] Found FirstPersonCharacter @ %p, fixing GunOffset",
           (void *)actor);
      // GunOffset @ 0x500 (Vector3: 0, 0, 10)
      writeFloat(actor + 0x500, 0.0f);
      writeFloat(actor + 0x500 + 4, 0.0f);
      writeFloat(actor + 0x500 + 8, 10.0f);
    }

    // 2. 找到 PlayerController -> 保存
    if (name.find("PlayerController") != std::string::npos) {
      LOGI("[!] Found PlayerController @ %p", (void *)actor);
      g_PlayerController = actor;
    }
  }
}

// 导出给 main.cpp 调用的入口
void hack_thread() {
  LOGI("============================================");
  LOGI("[ShadowHook_Check] 线程已启动，准备加载模块...");
  LOGI("============================================");

  // 初始化 ShadowHook
  // 尝试 UNIQUE 模式 (1)
  if (!initShadowHook(SHADOWHOOK_MODE_UNIQUE)) {
    LOGE("[!] ShadowHook 初始化失败，无法继续");
    return;
  }

  uintptr_t base = 0;
  while ((base = getModuleBase("libUE4.so")) == 0) {
    LOGD("[*] 正在检索 libUE4.so 基址...");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  LOGI("[*] 成功定位 libUE4.so 基址: %p", (void *)base);

  // 1. 先安装 Hook（和 JS 脚本顺序一致）
  // 这样从一开始就能捕获所有 SpawnProjectile 调用
  hookSpawnProjectile(base);

  // 2. 运行游戏逻辑循环 (找 Actor, 修复 GunOffset, 找 PlayerController)
  gameLogicLoop(base);

  LOGI("[*] gameLogicLoop 完成: PlayerController=%p",
       (void *)g_PlayerController);
}
