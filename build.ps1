# ============================================
# Zygisk Module Build Script
# Usage: .\build.ps1
# ============================================

$ErrorActionPreference = "Stop"

# ========== Configuration ==========
# NDK path (auto-detected)
$ndkPath = "$env:LOCALAPPDATA\Android\Sdk\ndk\29.0.14206865"

# If the path above is wrong, manually set it here:
# $ndkPath = "C:\Your\NDK\Path"

if (-not (Test-Path $ndkPath)) {
    Write-Host "[ERROR] NDK not found at: $ndkPath" -ForegroundColor Red
    Write-Host "Please edit build.ps1 and set the correct NDK path" -ForegroundColor Yellow
    exit 1
}

$ndkBuild = Join-Path $ndkPath "ndk-build.cmd"
$projectDir = $PSScriptRoot # $PSScriptRoot是当前脚本的目录
$moduleDir = Join-Path $projectDir "module"
$outputZip = Join-Path $projectDir "HelloZygisk.zip"

Write-Host "============================================" -ForegroundColor Cyan
Write-Host " Zygisk Module Builder v1.0" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "[*] Project: $projectDir" -ForegroundColor Gray
Write-Host "[*] NDK: $ndkPath" -ForegroundColor Gray
Write-Host ""

# ========== Step 1: Compile ==========
Write-Host "[1/3] Compiling C++ code..." -ForegroundColor Yellow

Push-Location $projectDir # Push-Location是将当前目录压入栈中
# 因为ndk-build需要在项目目录下执行，否则会找不到Android.mk
# 压入PowerShell的栈，压栈后，当前目录会变成项目目录
try {
    & $ndkBuild NDK_PROJECT_PATH=$projectDir APP_BUILD_SCRIPT="$projectDir\jni\Android.mk" NDK_APPLICATION_MK="$projectDir\jni\Application.mk" -j4 # j4是并行编译的线程数
    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed"
    }
    Write-Host "[OK] Compilation successful!" -ForegroundColor Green
}
catch {
    Write-Host "[FAIL] Compilation failed: $_" -ForegroundColor Red
    Pop-Location
    exit 1
}
Pop-Location

# ========== Step 2: Copy .so files ==========
Write-Host "[2/3] Assembling module files..." -ForegroundColor Yellow

$zygiskDir = Join-Path $moduleDir "zygisk" # moduleDir是module目录
if (-not (Test-Path $zygiskDir)) {
    New-Item -ItemType Directory -Path $zygiskDir -Force | Out-Null
}

# Copy compiled .so file
$soFile = Join-Path $projectDir "libs\arm64-v8a\libhellozygisk.so"
if (Test-Path $soFile) {
    Copy-Item $soFile (Join-Path $zygiskDir "arm64-v8a.so") -Force
    Write-Host "   [OK] Copied arm64-v8a.so" -ForegroundColor Gray
}
else {
    Write-Host "   [WARN] arm64-v8a .so file not found" -ForegroundColor Yellow
}

# Copy other architectures if available
$otherArchs = @("armeabi-v7a", "x86", "x86_64") # @是数组
foreach ($arch in $otherArchs) {
    $archSo = Join-Path $projectDir "libs\$arch\libhellozygisk.so"
    if (Test-Path $archSo) {
        Copy-Item $archSo (Join-Path $zygiskDir "$arch.so") -Force
        Write-Host "   [OK] Copied $arch.so" -ForegroundColor Gray
    }
}

Write-Host "[OK] Module files assembled!" -ForegroundColor Green

# ========== Step 3: Package ZIP ==========
Write-Host "[3/3] Packaging Magisk module..." -ForegroundColor Yellow

# Delete old zip
if (Test-Path $outputZip) {
    Remove-Item $outputZip -Force
}

# Use tar instead of Compress-Archive to get correct path separators
Push-Location $moduleDir
tar -a -cf $outputZip * # -a是归档，-c是创建，-f是文件
Pop-Location

if (Test-Path $outputZip) {
    $zipSize = (Get-Item $outputZip).Length / 1KB
    Write-Host "[OK] Package complete!" -ForegroundColor Green
    Write-Host ""
    Write-Host "============================================" -ForegroundColor Cyan
    Write-Host " BUILD SUCCESSFUL!" -ForegroundColor Green
    Write-Host "============================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "[*] Output: $outputZip" -ForegroundColor White
    Write-Host "[*] Size: $([math]::Round($zipSize, 2)) KB" -ForegroundColor White
    # Round是四舍五入
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Yellow
    Write-Host "   1. adb push HelloZygisk.zip /sdcard/Download/" -ForegroundColor Gray
    Write-Host "   2. Magisk App -> Modules -> Install from storage" -ForegroundColor Gray
    Write-Host "   3. Reboot phone" -ForegroundColor Gray
    Write-Host "   4. adb logcat -s HelloZygisk" -ForegroundColor Gray
}
else {
    Write-Host "[FAIL] Packaging failed" -ForegroundColor Red
    exit 1
}
