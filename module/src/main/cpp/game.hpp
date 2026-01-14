#pragma once
#include <cstdint>

// Start: 游戏数据定义区域 (Green Zone - definitions / Red Zone - values)
// 定义 UE4 的基础结构和 Offsets

namespace Game {

// [Interview Trap] 这里的 Offsets 必须和版本严格对应
// 来源: solve_aim.js
constexpr uintptr_t GWorld_Offset = 0xAFAC398;
constexpr uintptr_t GName_Offset = 0xADF07C0;
constexpr uintptr_t SpawnProjectile_Func_Offset = 0x8D2ED80;

// 简单向量结构
struct Vector3 {
  float x;
  float y;
  float z;
};

struct Rotator {
  float Pitch;
  float Yaw;
  float Roll;
};

// 常用函数声明
// void getName(uintptr_t objAddr); // 稍后在 hacks.cpp 实现
} // namespace Game
