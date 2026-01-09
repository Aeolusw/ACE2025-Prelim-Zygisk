/* * 进阶验证脚本 - 修复子弹乱飞 (GunOffset) + 修复射击方向 (SpawnRotation)
 * 逻辑来源: WriteUp [Source: 258, 442]
 */

var GWorld_Offset = 0xAFAC398;
var GName_Offset = 0xADF07C0;

// 关键函数地址 (SpawnActor/Projectile) [Source: 258]
var SpawnProjectile_Func_Offset = 0x8D2ED80;

var GName = null;
var moduleBase = null;
var g_PlayerController = null; // 全局保存 PlayerController 地址

// --- UE4 名字解析 ---
function getName(objAddr) {
    if (objAddr.isNull()) return "None";
    try {
        var fNameId = objAddr.add(0x18).readU32();
        var block = fNameId >> 16;
        var offset = fNameId & 65535;
        var fNamePool = GName.add(0x30);
        var chunk = fNamePool.add(0x10 + block * 8).readPointer();
        var entry = chunk.add(2 * offset);
        var header = entry.readU16();
        var len = header >> 6;
        if (len > 0 && len < 100) {
            return entry.add(2).readUtf8String(len);
        }
    } catch (e) { }
    return "None";
}

function main() {
    console.log("[*] 等待 libUE4.so 加载...");
    var waitLib = setInterval(function () {
        var lib = Process.findModuleByName("libUE4.so");
        if (lib) {
            clearInterval(waitLib);
            moduleBase = lib.base;
            console.log(`[*] libUE4 基址: ${moduleBase}`);

            // 开启 Hook，在“游戏逻辑开始运行”之前或者“刚刚开始”时就准备好
            hookSpawnProjectile();

            waitForGWorld(moduleBase);
        }
    }, 500);
}

function waitForGWorld(base) {
    GName = base.add(GName_Offset);
    var waitGWorld = setInterval(function () {
        try {
            var pGWorld = base.add(GWorld_Offset).readPointer();
            if (!pGWorld.isNull()) {
                clearInterval(waitGWorld);
                console.log(`[*] GWorld 已初始化: ${pGWorld}`);
                onReady(base, pGWorld);
            }
        } catch (e) { }
    }, 1000);
}

function onReady(base, pGWorld) {
    var level = pGWorld.add(0x30).readPointer();
    var actorsArray = level.add(0x98).readPointer();
    var actorsCount = level.add(0x98 + 8).readU32();

    console.log(`[*] 遍历 ${actorsCount} 个 Actor...`);

    for (var i = 0; i < actorsCount; i++) {
        var actor = actorsArray.add(i * 8).readPointer();
        if (actor.isNull()) continue;

        var name = getName(actor);

        // 1. 找到 FirstPersonCharacter -> 修复 GunOffset (子弹发射点)
        if (name.indexOf("FirstPersonCharacter") !== -1) {
            console.log(`✅ 找到角色: ${name} @ ${actor}`);
            // 修复 GunOffset (偏移 0x500) [Source: 354]
            // 原值 (100, 0, 10) -> 修正为 (0, 0, 10)
            actor.add(0x500).writeFloat(0.0);
            actor.add(0x500 + 4).writeFloat(0.0);
            actor.add(0x500 + 8).writeFloat(10.0);
            console.log("   -> GunOffset 已修正 (解决子弹乱飞)");
        }

        // 2. 找到 PlayerController -> 用于获取准星朝向
        if (name.indexOf("PlayerController") !== -1) {
            g_PlayerController = actor;
            console.log(`✅ 找到控制器: ${name} @ ${actor}`);
            console.log("   -> 已锁定 ControlRotation 源");
        }
    }
}

// --- 核心逻辑：Hook 子弹生成函数 ---
// [Source: 258, 442]
function hookSpawnProjectile() {
    var targetAddr = moduleBase.add(SpawnProjectile_Func_Offset);
    console.log(`[*] Hooking SpawnActor at ${targetAddr}`);

    Interceptor.attach(targetAddr, {
        onEnter: function (args) {
            // args[3] 是传入的 Rotation 指针 (原本是随机乱跳的值)
            var rotationPtr = args[3];

            if (g_PlayerController && !g_PlayerController.isNull()) {
                // 获取玩家当前的准星朝向 (ControlRotation)
                // 偏移 0x288 是 PlayerController 的 ControlRotation [Source: 349]
                var ctrlRotPtr = g_PlayerController.add(0x288);

                var pitch = ctrlRotPtr.readFloat();
                var yaw = ctrlRotPtr.add(4).readFloat();
                // var roll = ctrlRotPtr.add(8).readFloat(); // 通常不需要roll

                // 【关键一步】强行覆盖传入参数
                // 让子弹生成的角度 = 玩家准星的角度
                rotationPtr.writeFloat(pitch);
                rotationPtr.add(4).writeFloat(yaw);

                // console.log(`[Shoot] 强制修正子弹角度 -> P:${pitch.toFixed(2)}, Y:${yaw.toFixed(2)}`);
            }
        }
    });
}

setImmediate(main);