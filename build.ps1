# ============================================
# Zygisk Module Build Script (Refined)
# Usage: .\build.ps1
# ============================================

$ErrorActionPreference = "Stop"

# ========== Configuration ==========
$ndkPath = "$env:LOCALAPPDATA\Android\Sdk\ndk\29.0.14206865"
if (-not (Test-Path $ndkPath)) {
    Write-Host "[ERROR] NDK not found at: $ndkPath" -ForegroundColor Red
    exit 1
}

# Find CMake & Ninja
function Get-CMakePath {
    $cmakePath = Get-Command cmake -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
    if ($cmakePath) { return $cmakePath }
    # Try Android SDK
    $sdkCmake = Get-ChildItem -Path "$env:LOCALAPPDATA\Android\Sdk\cmake" -Recurse -Filter cmake.exe -ErrorAction SilentlyContinue | Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName
    return $sdkCmake
}

$cmakeExe = Get-CMakePath
if (-not $cmakeExe) {
    Write-Host "[ERROR] CMake not found!" -ForegroundColor Red
    exit 1
}
# Add CMake bin to PATH for Ninja detection
$cmakeBin = Split-Path $cmakeExe -Parent
$env:PATH = "$cmakeBin;$env:PATH"

# Paths
$projectDir = $PSScriptRoot
$cppDir     = Join-Path $projectDir "module\src\main\cpp"
$buildRoot  = Join-Path $projectDir "obj"
$outputZip  = Join-Path $projectDir "HelloZygisk.zip"
$moduleDir  = Join-Path $projectDir "module"
$zygiskDir  = Join-Path $moduleDir  "zygisk"

# Toolchain file (ensure forward slashes or correct path)
$toolchain = Join-Path $ndkPath "build\cmake\android.toolchain.cmake"

Write-Host "============================================" -ForegroundColor Cyan
Write-Host " Zygisk Builder (Stable)" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "CMake: $cmakeExe"
Write-Host "NDK:   $ndkPath"
Write-Host ""

# ========== Build Loop ==========
$abis = @("arm64-v8a") # Add others if needed: "armeabi-v7a", "x86", "x86_64"

foreach ($abi in $abis) {
    Write-Host ">>> Building for $abi..." -ForegroundColor Yellow
    $buildDir = Join-Path $buildRoot $abi
    
    # Clean previous build if needed (Optional: remove Force if too slow)
    # if (Test-Path $buildDir) { Remove-Item -Recurse -Force $buildDir }

    # 1. Configure (Using -S and -B for out-of-source build)
    # We pass all arguments explicitly
    $configArgs = @(
        "-G", "Ninja",
        "-S", $cppDir,
        "-B", $buildDir,
        "-DCMAKE_TOOLCHAIN_FILE=$toolchain",
        "-DANDROID_NDK=$ndkPath",
        "-DANDROID_ABI=$abi",
        "-DANDROID_PLATFORM=android-35",
        "-DANDROID_STL=c++_static",
        "-DCMAKE_BUILD_TYPE=Release"
    )
    
    try {
        & $cmakeExe $configArgs
        if ($LASTEXITCODE -ne 0) { throw "Configuration failed" }

        # 2. Build
        & $cmakeExe --build $buildDir
        if ($LASTEXITCODE -ne 0) { throw "Build failed" }
    }
    catch {
        Write-Host "[FAIL] $_" -ForegroundColor Red
        exit 1
    }

    # 3. Install
    $builtSo = Join-Path $buildDir "libhellozygisk.so"
    if (-not (Test-Path $zygiskDir)) { New-Item -ItemType Directory -Path $zygiskDir -Force | Out-Null }
    
    if (Test-Path $builtSo) {
        Copy-Item $builtSo (Join-Path $zygiskDir "$abi.so") -Force
        Write-Host "   [OK] Installed $abi.so" -ForegroundColor Green
    } else {
        Write-Host "   [ERR] .so file missing at $builtSo" -ForegroundColor Red
        exit 1
    }
}

# ========== Packaging ==========
if (Test-Path $outputZip) { Remove-Item $outputZip -Force }

Write-Host ">>> Packaging..." -ForegroundColor Yellow
pushd $moduleDir
tar -a -cf $outputZip *
popd

if (Test-Path $outputZip) {
    Write-Host "[SUCCESS] $outputZip" -ForegroundColor Green
} else {
    Write-Host "[FAIL] Zip creation failed" -ForegroundColor Red
}
