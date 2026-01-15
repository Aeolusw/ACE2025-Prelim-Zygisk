#pragma once
#include "shadowhook.h"
#include "utils.hpp"

// 模式: SHARED / UNIQUE
// SHADOWHOOK_MODE_UNIQUE (1): 同一地址只能被 Hook 一次
// SHADOWHOOK_MODE_SHARED (0): 同一地址可以被多次 Hook (需避免递归)
inline bool initShadowHook(int mode = SHADOWHOOK_MODE_UNIQUE) {
  // 初始化 ShadowHook
  // debuggable: true 输出更多日志，release 时建议为 false
  int ret = shadowhook_init((shadowhook_mode_t)mode, true);
  if (ret != 0) {
    LOGE("ShadowHook init failed: %d - %s", shadowhook_get_errno(),
         shadowhook_to_errmsg(shadowhook_get_errno()));
    return false;
  }
  LOGI("ShadowHook initialized successfully");
  return true;
}
