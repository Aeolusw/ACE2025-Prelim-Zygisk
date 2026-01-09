#include <android/log.h>  // 行1
#include <ctime>          // 行2

void printCurrentTime() {  // 行3
    time_t now = time(nullptr);           // 行4
    char* timeStr = ctime(&now);          // 行5
    __android_log_print(ANDROID_LOG_INFO, "MyZygisk", "Current Time: %s", timeStr);  // 行6
}  // 行7