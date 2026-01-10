# ACE2025-Prelim-Zygisk

2025 腾讯游戏安全竞赛安卓初赛复现

## 环境

JDK 17
NDK 26
Gradle 8.6

## 编译

**Android Studio**
直接运行 `packageZygisk` 任务

**命令行**
```shell
./gradlew :module:packageZygisk
```

产物 `HelloZygisk.zip` 生成于项目根目录

## 技术要点

**内存基址**
使用 `dl_iterate_phdr` 获取 `libUE4.so` 信息

**Hook 时机**
`postAppSpecialize` 阶段执行

**稳定性**
JNI 指针检查

## 声明

仅供安全研究
