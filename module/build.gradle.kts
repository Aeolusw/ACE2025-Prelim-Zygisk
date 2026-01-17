plugins {
    id("com.android.library")
}

android {
    namespace = "com.tencent.ace2025.prelim"
    compileSdk = 34
    ndkVersion = "26.1.10909125"

    defaultConfig {
        minSdk = 26
        
        externalNativeBuild {
            cmake {
                cppFlags("-std=c++20")
                arguments(
                    "-DANDROID_STL=c++_static",
                    "-DPROJECT_ROOT=${rootProject.projectDir}"
                )
                abiFilters("arm64-v8a")
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}

tasks.register<Zip>("packageZygisk") {
    dependsOn("assembleRelease")
    group = "magisk"
    description = "Package Zygisk module" // Fixed 'desciption' typo

    archiveFileName.set("HelloZygisk.zip")
    destinationDirectory.set(rootProject.layout.projectDirectory)

    from(file("module.prop"))
    
    into("zygisk") {
        from(layout.buildDirectory.dir("intermediates/stripped_native_libs/release/stripReleaseDebugSymbols/out/lib/arm64-v8a")) {
            include("libhellozygisk.so")
            rename { "arm64-v8a.so" }
        }
    }

    into("system/lib64") {
        from(layout.buildDirectory.dir("intermediates/stripped_native_libs/release/stripReleaseDebugSymbols/out/lib/arm64-v8a")) {
            include("libshadowhook_nothing.so")
        }
    }
    
    doLast {
        println("Packed to ${archiveFile.get().asFile.absolutePath}")
    }
}
