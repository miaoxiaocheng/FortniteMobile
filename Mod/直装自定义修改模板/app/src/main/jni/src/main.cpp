// 基于Fortnite终端的帧率与3D分辨率修改功能，使用Android-Memory-Debug.h
#include <jni.h>
#include "Android-Memory-Debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <android/log.h>
#include <time.h>
#include <cmath> // 用于fabs函数

// Logcat日志标签
#define LOG_TAG "FortniteHack"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

// 文件日志路径
#define LOG_FILE_PATH "/storage/emulated/0/Android/data/com.epicgames.fortnite/game_hack.log"

// 简化的文件日志函数
void write_file_log(const char* level, const char* tag, const char* format, ...) {
    char log_buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(log_buffer, sizeof(log_buffer), format, args);
    va_end(args);
    
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    char full_log[2048];
    snprintf(full_log, sizeof(full_log), "[%s] [%s] [%s] %s\n", time_str, level, tag, log_buffer);
    
    FILE* log_file = fopen(LOG_FILE_PATH, "a");
    if (log_file != NULL) {
        fprintf(log_file, "%s", full_log);
        fclose(log_file);
    }
}

#define SIMPLE_LOGE(tag, fmt, ...) do { \
    LOGE(fmt, ##__VA_ARGS__); \
    write_file_log("ERROR", tag, fmt, ##__VA_ARGS__); \
} while(0)

#define SIMPLE_LOGI(tag, fmt, ...) do { \
    LOGI(fmt, ##__VA_ARGS__); \
    write_file_log("INFO", tag, fmt, ##__VA_ARGS__); \
} while(0)

#define SIMPLE_LOGD(tag, fmt, ...) do { \
    LOGD(fmt, ##__VA_ARGS__); \
    write_file_log("DEBUG", tag, fmt, ##__VA_ARGS__); \
} while(0)

// 通过包名获取pid
int getPID(const char *packageName) {
    MemoryDebug mem;
    int pid = mem.setPackageName(packageName);
    if (pid > 0) {
        SIMPLE_LOGI("getPID", "找到游戏进程: %s, PID=%d", packageName, pid);
    } else {
        SIMPLE_LOGE("getPID", "未找到游戏进程: %s", packageName);
    }
    return pid;
}

// 获取 libUnreal.so:cbbss 地址 (用于帧率) - 来自Fortnite终端的正确方法
long int get_module_base_cbbss(int pid) {
    char filename[64];
    snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        SIMPLE_LOGE("get_module_base_cbbss", "无法打开maps文件");
        return 0;
    }

    char line[1024];
    int line_number = 0;
    int target_line_num = -1;
    
    // 第一步：查找最后一个同时包含"libUnreal.so"和"rw"的行 (即 Cd 段)
    while (fgets(line, sizeof(line), fp)) {
        line_number++;
        if (strstr(line, "libUnreal.so") && strstr(line, "rw")) {
            target_line_num = line_number;
        }
    }
    
    if (target_line_num == -1) {
        SIMPLE_LOGE("get_module_base_cbbss", "未找到Cd段");
        fclose(fp);
        return 0;
    }
    
    // 重新定位文件指针，查找Cd段的下一行 (Cb段)
    rewind(fp);
    line_number = 0;
    long int bss_addr = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        line_number++;
        if (line_number == target_line_num + 1) { // 检查Cd段的下一行
            // 检查：该行必须包含"rw"和BSS段标识"[anon:.bss]" (即 Cb 段)
            if (strstr(line, "rw") && strstr(line, "[anon:.bss]")) {
                // 提取BSS段的起始地址
                char addr_str[32];
                if (sscanf(line, "%31s", addr_str) == 1) {
                    bss_addr = strtoul(addr_str, NULL, 16);
                    SIMPLE_LOGD("get_module_base_cbbss", "找到cbbss段基址: 0x%lx", bss_addr);
                }
            }
            break;
        }
    }
    
    fclose(fp);
    
    if (bss_addr == 0) {
        SIMPLE_LOGE("get_module_base_cbbss", "未找到cbbss段");
    }
    
    return bss_addr; // 返回 libUnreal.so:cbbss 地址
}

extern "C" 
{
//------------------------------------------------------------------------------------------------------------------------------------------
// 功能1：修改帧率 (从文档2移植，保持不变)
JNIEXPORT jboolean JNICALL Java_load_tencent_lib_FloatContentView_aw1(JNIEnv *env, jobject obj, jfloat inputValue) {
    SIMPLE_LOGI("aw1", "函数被调用，输入值: %f", inputValue);
    
    // 将float值转换为int（DWORD类型）
    int value = (int)inputValue;
    SIMPLE_LOGI("aw1", "转换后的DWORD值: %d (十六进制: 0x%X)", value, value);
    
    // 1. 获取游戏进程ID
    int ipid = getPID("com.epicgames.fortnite");
    SIMPLE_LOGI("aw1", "游戏进程PID: %d", ipid);
    
    if (ipid <= 0) {
        SIMPLE_LOGE("aw1", "错误：未找到游戏进程 com.epicgames.fortnite");
        return JNI_FALSE;
    }
    
    // 2. 创建MemoryDebug实例
    MemoryDebug mem;
    mem.setPackageName("com.epicgames.fortnite");
    
    // 3. 获取帧率功能基址
    SIMPLE_LOGI("aw1", "正在获取libUnreal.so:cbbss基址");
    long int base_cbbss = get_module_base_cbbss(ipid);
    SIMPLE_LOGI("aw1", "cbbss基址: 0x%lx", base_cbbss);
    
    if (base_cbbss == 0) {
        SIMPLE_LOGE("aw1", "错误：未找到帧率功能基址 (libUnreal.so:cbbss)");
        return JNI_FALSE;
    }
    
    // 4. 计算帧率地址：cbbss基址 + 0xFD8D8
    long int fps_address = base_cbbss + 0xFD8D8;
    SIMPLE_LOGI("aw1", "计算帧率地址: 0x%lx (cbbss基址 0x%lx + 偏移 0xFD4D0)", fps_address, base_cbbss);
    
    // 5. 使用MemoryDebug读取当前帧率
    int current_fps = 0;
    ssize_t readResult = mem.preadv(fps_address, &current_fps, 4);
    if (readResult == 4) {
        SIMPLE_LOGI("aw1", "地址 0x%lx 当前存储的DWORD值: %d (十六进制: 0x%X)", 
                   fps_address, current_fps, (unsigned int)current_fps);
    } else {
        SIMPLE_LOGE("aw1", "读取当前帧率失败，返回: %zd", readResult);
        return JNI_FALSE;
    }
    
    // 6. 使用MemoryDebug写入新的帧率值
    SIMPLE_LOGI("aw1", "正在写入DWORD值 %d 到地址 0x%lx", value, fps_address);
    
    // 使用MemoryDebug的edit方法写入
    int write_result = mem.edit(value, fps_address, DWORD, true);
    if (write_result == 1) {
        SIMPLE_LOGI("aw1", "写入操作完成");
    } else {
        SIMPLE_LOGE("aw1", "写入操作失败");
        return JNI_FALSE;
    }
    
    // 7. 验证写入结果
    int verify_fps = 0;
    mem.preadv(fps_address, &verify_fps, 4);
    SIMPLE_LOGI("aw1", "验证地址 0x%lx 的值: %d (十六进制: 0x%X)", 
               fps_address, verify_fps, (unsigned int)verify_fps);
    
    if (verify_fps == value) {
        SIMPLE_LOGI("aw1", "✅ 写入验证成功！");
        return JNI_TRUE;
    } else {
        SIMPLE_LOGE("aw1", "❌ 写入验证失败，期望值: %d, 实际值: %d", value, verify_fps);
        return JNI_FALSE;
    }
}
//------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------
// 功能2：修改3D分辨率 (从文档1移植，函数名改为aw2)
JNIEXPORT void JNICALL Java_load_tencent_lib_FloatContentView_aw2(JNIEnv *env, jobject obj, jfloat inputValue) {
    SIMPLE_LOGI("aw2", "===== 开始内存修改过程 (3D分辨率修改) =====");
    SIMPLE_LOGI("aw2", "JNI函数aw2被调用，输入值:%f", inputValue);
    
    float value = inputValue;
    
    // 1. 获取游戏进程ID
    SIMPLE_LOGI("aw2", "开始获取进程PID");
    int ipid = getPID("com.epicgames.fortnite");
    if (ipid <= 0) {
        SIMPLE_LOGE("aw2", "获取PID失败");
        SIMPLE_LOGI("aw2", "===== 内存修改过程失败结束 =====");
        return;
    }
    SIMPLE_LOGI("aw2", "获取PID成功: %d", ipid);
    
    // 创建MemoryDebug实例
    MemoryDebug memDebug;
    memDebug.setPackageName("com.epicgames.fortnite");
    
    // 2. 获取cbbss基址
    SIMPLE_LOGI("aw2", "开始获取模块基址 (libUnreal.so)");
    const char* mname = "libUnreal.so";
    long int fool = get_module_base_cbbss(ipid);
    if (fool == 0) {
        SIMPLE_LOGE("aw2", "获取cbbss基址失败");
        SIMPLE_LOGI("aw2", "===== 内存修改过程失败结束 =====");
        return;
    }
    SIMPLE_LOGI("aw2", "获取cbbss基址成功: 0x%lx", fool);
    
    // 步骤1: 计算第一级地址 (基址 + 偏移)
    long int firstLevelAddress = fool + 0x169BC0;
    SIMPLE_LOGI("aw2", "计算第一级地址: 基址0x%lx + 偏移0x169BC0 = 0x%lx", fool, firstLevelAddress);
    
    // 步骤2: 从第一级地址读取64位指针
    SIMPLE_LOGI("aw2", "从第一级地址读取64位指针");
    long int pointer64 = memDebug.ReadLong(firstLevelAddress);
    if (pointer64 == 0) {
        SIMPLE_LOGE("aw2", "读取64位指针失败");
        SIMPLE_LOGI("aw2", "===== 内存修改过程失败结束 =====");
        return;
    }
    SIMPLE_LOGI("aw2", "从第一级地址0x%lx使用ReadLong读取到64位指针:0x%lx", firstLevelAddress, pointer64);
    
    // 步骤3: 跳转到64位指针指向的地址
    long int finalAddress = pointer64;  // 直接跳转到指针指向的地址
    SIMPLE_LOGI("aw2", "最终地址: 跳转到指针0x%lx指向的地址 (3D分辨率地址)", finalAddress);
    
    // 步骤4: 读取最终地址的旧值
    SIMPLE_LOGI("aw2", "开始读取最终地址的内存旧值");
    float oldValue = memDebug.ReadFloat(finalAddress);
    SIMPLE_LOGI("aw2", "最终地址:0x%lx, 旧值:%f", finalAddress, oldValue);
    
    // 步骤5: 修改最终地址的内存值
    SIMPLE_LOGI("aw2", "开始修改最终地址的内存值");
    SIMPLE_LOGI("aw2", "最终地址:0x%lx, 旧值:%f, 新值:%f", finalAddress, oldValue, value);
    int result = memDebug.edit(value, finalAddress, FLOAT, false);
    
    if (result != 1) {
        SIMPLE_LOGE("aw2", "edit函数修改失败");
        // 如果edit失败，尝试使用pwritev进行低级写入
        SIMPLE_LOGI("aw2", "尝试使用pwritev进行低级写入");
        size_t writeResult = memDebug.pwritev(finalAddress, &value, sizeof(float));
        if (writeResult != sizeof(float)) {
            SIMPLE_LOGE("aw2", "pwritev写入失败");
            SIMPLE_LOGI("aw2", "===== 内存修改过程失败结束 =====");
            return;
        }
    } else {
        SIMPLE_LOGI("aw2", "edit函数修改成功");
    }
    
    // 步骤6: 验证修改
    SIMPLE_LOGI("aw2", "开始验证内存修改");
    float verifyValue = memDebug.ReadFloat(finalAddress);
    bool verifySuccess = fabs(verifyValue - value) < 0.001f;
    
    SIMPLE_LOGI("aw2", "期望值:%f, 实际值:%f, 差值:%f", value, verifyValue, fabs(verifyValue - value));
    
    if (verifySuccess) {
        SIMPLE_LOGI("aw2", "内存修改验证成功 - 3D分辨率已修改");
        // 记录完整的指针链信息
        SIMPLE_LOGI("aw2", "指针链完整信息:");
        SIMPLE_LOGI("aw2", "1. 基址: 0x%lx (通过cbbss方法获取)", fool);
        SIMPLE_LOGI("aw2", "2. 第一级地址: 0x%lx (基址 + 0x169BC0)", firstLevelAddress);
        SIMPLE_LOGI("aw2", "3. 64位指针: 0x%lx (通过ReadLong函数读取)", pointer64);
        SIMPLE_LOGI("aw2", "4. 最终地址: 0x%lx (跳转指针，3D分辨率地址)", finalAddress);
        SIMPLE_LOGI("aw2", "5. 旧值: %f (通过ReadFloat函数读取)", oldValue);
        SIMPLE_LOGI("aw2", "6. 新值: %f (通过edit函数写入)", value);
    } else {
        SIMPLE_LOGE("aw2", "内存修改验证失败");
        // 尝试通过preadv读取验证
        float directReadValue = 0;
        size_t readResult = memDebug.preadv(finalAddress, &directReadValue, sizeof(float));
        if (readResult == sizeof(float)) {
            SIMPLE_LOGI("aw2", "通过preadv直接读取: 地址0x%lx, 值:%f", finalAddress, directReadValue);
        }
    }
    
    SIMPLE_LOGI("aw2", "===== 内存修改过程完成 =====");
}
//------------------------------------------------------------------------------------------------------------------------------------------
}
