buildscript {
    repositories {
        google()
        mavenCentral()
    }
    dependencies {
        classpath("com.android.tools.build:gradle:8.4.0")
    }
}

tasks.register<Delete>("clean") {
    delete(rootProject.buildDir)
}
