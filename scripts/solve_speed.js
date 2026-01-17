/**
 * 独立脚本 - 修复移动速度过快问题
 * Offsets: [Source: NotebookLM WriteUp]
 *   - CharacterMovementComponent: Actor + 0x288
 *   - MaxWalkSpeed: Component + 0x18c
 *   - MaxAcceleration: Component + 0x1a0
 */

// ==================== 常量偏移 ====================
const GWorld_Offset = 0xAFAC398;
const GName_Offset = 0xADF07C0;

const OFFSET_LEVEL = 0x30;
const OFFSET_ACTORS_ARRAY = 0x98;
const OFFSET_FNAME_ID = 0x18;

const TARGET_SPEED = 600.0;

// ==================== 全局状态 ====================
let GName = null;
let moduleBase = null;

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

function findActorByName(pGWorld, namePattern) {
    const level = pGWorld.add(OFFSET_LEVEL).readPointer();
    const actorsArray = level.add(OFFSET_ACTORS_ARRAY).readPointer();
    const actorsCount = level.add(OFFSET_ACTORS_ARRAY + 8).readU32();

    for (let i = 0; i < actorsCount; i++) {
        const actor = actorsArray.add(i * 8).readPointer();
        if (actor.isNull()) continue;
        if (getName(actor).indexOf(namePattern) !== -1) {
            return actor;
        }
    }
    return null;
}

function readGWorld() {
    try {
        const p = moduleBase.add(GWorld_Offset).readPointer();
        return p.isNull() ? null : p;
    } catch (e) {
        return null;
    }
}

// ==================== 主逻辑 ====================

function main() {
    console.log("[*] 速度修复脚本启动...");

    const waitLib = setInterval(() => {
        const lib = Process.findModuleByName("libUE4.so");
        if (lib) {
            clearInterval(waitLib);
            moduleBase = lib.base;
            GName = moduleBase.add(GName_Offset);
            console.log(`[*] libUE4 基址: ${moduleBase}`);
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

                fixSpeed(pGWorld);

                // 持续修复 (防止游戏线程覆盖)
                setInterval(() => {
                    const currentGWorld = readGWorld();
                    if (currentGWorld) {
                        fixSpeed(currentGWorld);
                    }
                }, 1000);
            }
        } catch (e) { }
    }, 1000);
}

function fixSpeed(pGWorld) {
    const character = findActorByName(pGWorld, "FirstPersonCharacter");
    if (!character) return;

    const movementComp = character.add(0x288).readPointer();
    if (movementComp.isNull()) return;

    const oldSpeed = movementComp.add(0x18c).readFloat();
    const oldAccel = movementComp.add(0x1a0).readFloat();

    if (oldSpeed > TARGET_SPEED + 100 || oldAccel > TARGET_SPEED + 100) {
        movementComp.add(0x18c).writeFloat(TARGET_SPEED);
        movementComp.add(0x1a0).writeFloat(TARGET_SPEED);

        console.log(`✅ 速度已修复: MaxWalkSpeed ${oldSpeed.toFixed(0)} -> ${TARGET_SPEED}`);
        console.log(`   MaxAcceleration ${oldAccel.toFixed(0)} -> ${TARGET_SPEED}`);
    }
}

setImmediate(main);
