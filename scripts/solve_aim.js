/**
 * 独立脚本 - 修复子弹乱飞 (GunOffset) + 修复射击方向 (SpawnRotation)
 * 逻辑来源: WriteUp [Source: 258, 442]
 */

// ==================== 常量偏移 ====================
const GWorld_Offset = 0xAFAC398;
const GName_Offset = 0xADF07C0;
const SpawnProjectile_Func_Offset = 0x8D2ED80;

const OFFSET_LEVEL = 0x30;
const OFFSET_ACTORS_ARRAY = 0x98;
const OFFSET_FNAME_ID = 0x18;

// ==================== 全局状态 ====================
let GName = null;
let moduleBase = null;
let g_PlayerController = null;

// ==================== 工具函数 ====================

function getName(objAddr) {
    if (objAddr.isNull()) return "None";
    try {
        const fNameId = objAddr.add(OFFSET_FNAME_ID).readU32();
        const block = fNameId >> 16;
        const offset = fNameId & 65535;
        const fNamePool = GName.add(0x30);
        const chunk = fNamePool.add(0x10 + block * 8).readPointer();
        const entry = chunk.add(2 * offset);
        const header = entry.readU16();
        const len = header >> 6;
        if (len > 0 && len < 100) {
            return entry.add(2).readUtf8String(len);
        }
    } catch (e) { }
    return "None";
}

function forEachActor(pGWorld, callback) {
    const level = pGWorld.add(OFFSET_LEVEL).readPointer();
    const actorsArray = level.add(OFFSET_ACTORS_ARRAY).readPointer();
    const actorsCount = level.add(OFFSET_ACTORS_ARRAY + 8).readU32();

    for (let i = 0; i < actorsCount; i++) {
        const actor = actorsArray.add(i * 8).readPointer();
        if (actor.isNull()) continue;
        callback(actor, getName(actor));
    }
}

// ==================== 主逻辑 ====================

function main() {
    console.log("[*] 瞄准修复脚本启动...");

    const waitLib = setInterval(() => {
        const lib = Process.findModuleByName("libUE4.so");
        if (lib) {
            clearInterval(waitLib);
            moduleBase = lib.base;
            GName = moduleBase.add(GName_Offset);
            console.log(`[*] libUE4 基址: ${moduleBase}`);

            hookSpawnProjectile();
            waitForGWorld();
        }
    }, 500);
}

function waitForGWorld() {
    const wait = setInterval(() => {
        try {
            const pGWorld = moduleBase.add(GWorld_Offset).readPointer();
            if (!pGWorld.isNull()) {
                clearInterval(wait);
                console.log(`[*] GWorld 已初始化: ${pGWorld}`);
                onReady(pGWorld);
            }
        } catch (e) { }
    }, 1000);
}

function onReady(pGWorld) {
    console.log("[*] 开始遍历 Actor...");

    forEachActor(pGWorld, (actor, name) => {
        if (name.indexOf("FirstPersonCharacter") !== -1) {
            console.log(`✅ 找到角色: ${name} @ ${actor}`);
            // 修复 GunOffset (偏移 0x500)
            actor.add(0x500).writeFloat(0.0);
            actor.add(0x500 + 4).writeFloat(0.0);
            actor.add(0x500 + 8).writeFloat(10.0);
            console.log("   -> GunOffset 已修正 (解决子弹乱飞)");
        }

        if (name.indexOf("PlayerController") !== -1) {
            g_PlayerController = actor;
            console.log(`✅ 找到控制器: ${name} @ ${actor}`);
            console.log("   -> 已锁定 ControlRotation 源");
        }
    });
}

function hookSpawnProjectile() {
    const targetAddr = moduleBase.add(SpawnProjectile_Func_Offset);
    console.log(`[*] Hooking SpawnProjectile at ${targetAddr}`);

    Interceptor.attach(targetAddr, {
        onEnter(args) {
            const rotationPtr = args[3];

            if (g_PlayerController && !g_PlayerController.isNull()) {
                const ctrlRotPtr = g_PlayerController.add(0x288);
                const pitch = ctrlRotPtr.readFloat();
                const yaw = ctrlRotPtr.add(4).readFloat();

                rotationPtr.writeFloat(pitch);
                rotationPtr.add(4).writeFloat(yaw);
            }
        }
    });
}

setImmediate(main);
