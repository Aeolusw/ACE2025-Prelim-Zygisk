# ACE2025-Prelim-Zygisk

使用 Zygisk 复现 2025 年腾讯游戏安全竞赛安卓赛道初赛

## 技术要点

- **基址获取**
  使用 `dl_iterate_phdr` 遍历 Linker 链表获取 `libUE4.so` 基址，替代 `/proc/self/maps` 解析
- **生命周期处理**
  Hook 逻辑移至 `postAppSpecialize` 阶段执行，避免阻塞主线程初始化
- **稳定性**
  增加 JNI 指针检查，防止空指针导致进程崩溃
- **环境**
  NDK r25+ / C++20

## 使用方法

1. **编译**
   运行 `.\build.ps1`
2. **安装**
   将 `HelloZygisk.zip` 传入手机，在 Magisk 中安装并重启
3. **验证**
   查看日志确认模块加载：
   `adb logcat -s Zygisk_UE4_Aim`

## 声明

仅供安全研究用途
