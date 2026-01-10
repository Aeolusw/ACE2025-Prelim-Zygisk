plugins {
    id("com.android.library")
}

android {
    namespace = "com.tencent.ace2025.client"
    compileSdk = 34
    ndkVersion = "26.1.10909125"

    defaultConfig {
        minSdk = 26
        targetSdk = 30
        
        externalNativeBuild {
            cmake {
                cppFlags("-std=c++20")
                arguments("-DANDROID_STL=c++_static")
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
        // Path to the stripped native library
        from(layout.buildDirectory.dir("intermediates/native_libs/release/out/lib/arm64-v8a")) {
             // In some AGP versions it might be different, but native_libs or stripped_native_libs is common.
             // Using 'native_libs' is safer than 'stripped' if we want to be sure it exists, 
             // but 'stripped' is better for size. Let's try stripped first.
        }
        from(layout.buildDirectory.dir("intermediates/stripped_native_libs/release/out/lib/arm64-v8a")) {
            include("**/*.so")
            rename { "arm64-v8a.so" }
        }
    }
    
    doLast {
        println("Packed to ${archiveFile.get().asFile.absolutePath}")
    }
}
