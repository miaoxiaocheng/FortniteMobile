#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <string.h>
#include <math.h>

// 全局变量
static long int handle = 0;
// 帧率相关变量
static long int fps_address = 0;
static int target_fps = 120;            // 默认目标帧率
static int lock_fps_enabled = 0;        // 帧率锁定状态
static int lock_check_interval_fps = 10;// 帧率检测间隔
static pthread_t lock_fps_thread_tid;
// 3D分辨率相关变量
static long int resolution_3d_address = 0;
static float current_3d_res = 0.0f;     // 当前3D分辨率
static float target_3d_res = 100.0f;    // 默认目标3D分辨率 (100%)
static int lock_res_enabled = 0;        // 3D分辨率锁定状态
static int lock_check_interval_res = 10;// 分辨率检测间隔
static pthread_t lock_res_thread_tid;

// 获取进程ID
int getPID(const char *packageName) {
    DIR *dir = opendir("/proc");
    if (!dir) return -1;

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        int id = atoi(entry->d_name);
        if (id > 0) {
            char filename[64], cmdline[64];
            sprintf(filename, "/proc/%d/cmdline", id);

            FILE *fp = fopen(filename, "r");
            if (fp) {
                fgets(cmdline, sizeof(cmdline), fp);
                fclose(fp);
                if (strcmp(packageName, cmdline) == 0) {
                    closedir(dir);
                    return id;
                }
            }
        }
    }
    closedir(dir);
    return -1;
}

// 修改点1: 获取 libUnreal.so:cbbss 地址 (用于帧率)
long int get_module_base_cbbss(int pid) {
    char filename[64];
    snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;

    char line[1024];
    int line_number = 0;
    int target_line_num = -1;
    // 第一步：查找最后一个同时包含"libUnreal.so"和"rw-p"的行 (即 Cd 段)
    while (fgets(line, sizeof(line), fp)) {
        line_number++;
        if (strstr(line, "libUnreal.so") && strstr(line, "rw-p")) {
            target_line_num = line_number;
        }
    }
    if (target_line_num == -1) {
        fclose(fp);
        return 0; // 未找到Cd段
    }
    // 重新定位文件指针，查找Cd段的下一行 (Cb段)
    rewind(fp);
    line_number = 0;
    long int bss_addr = 0;
    while (fgets(line, sizeof(line), fp)) {
        line_number++;
        if (line_number == target_line_num + 1) { // 检查Cd段的下一行
            // 检查：该行必须包含"rw-p"和BSS段标识"[anon:.bss]" (即 Cb 段)
            if (strstr(line, "rw-p") && strstr(line, "[anon:.bss]")) {
                // 提取BSS段的起始地址（第一个字段，16进制字符串）
                char addr_str[32];
                if (sscanf(line, "%31s", addr_str) == 1) {
                    bss_addr = strtoul(addr_str, NULL, 16);
                }
            }
            break;
        }
    }
    fclose(fp);
    return bss_addr; // 返回 libUnreal.so:cbbss 地址
}

// 通用内存读取函数 (32位整数)
int mem_read_int(long int addr) {
    int value = 0;
    if (pread64(handle, &value, 4, addr) != 4) {
        printf("读取内存失败，游戏可能已退出\n");
        exit(1);
    }
    return value;
}

// 通用内存读取函数 (32位浮点数)
float mem_read_float(long int addr) {
    float value = 0.0f;
    if (pread64(handle, &value, 4, addr) != 4) {
        printf("读取内存失败，游戏可能已退出\n");
        exit(1);
    }
    return value;
}

// 通用内存写入函数 (32位整数)
int mem_write_int(long int addr, int value) {
    return pwrite64(handle, &value, 4, addr) == 4 ? 0 : -1;
}

// 通用内存写入函数 (32位浮点数)
int mem_write_float(long int addr, float value) {
    return pwrite64(handle, &value, 4, addr) == 4 ? 0 : -1;
}

// 新增函数：读取64位内存值
long int mem_read_long(long int addr) {
    uint64_t value = 0;
    if (pread64(handle, &value, 8, addr) != 8) {
        printf("读取64位内存失败，游戏可能已退出\n");
        exit(1);
    }
    return (long int)value;
}

// 读取当前帧率
int read_current_fps() {
    return mem_read_int(fps_address);
}

// 读取当前3D分辨率
float read_current_3d_res() {
    return mem_read_float(resolution_3d_address);
}

// 帧率锁定线程函数
void *lock_fps_thread_func(void *arg) {
    int check_count = 0;
    while (lock_fps_enabled) {
        int current_fps = read_current_fps();
        if (current_fps != target_fps) {
            mem_write_int(fps_address, target_fps);
            printf("[FPS锁定#%d] 修正 %d → %d FPS\n", ++check_count, current_fps, target_fps);
        } else if (check_count % 10 == 0) {
            printf("[FPS锁定#%d] 正常 %d FPS\n", ++check_count, current_fps);
        } else {
            check_count++;
        }
        sleep(lock_check_interval_fps);
    }
    return NULL;
}

// 3D分辨率锁定线程函数
void *lock_res_thread_func(void *arg) {
    int check_count = 0;
    while (lock_res_enabled) {
        float current_res = read_current_3d_res();
        // 使用一个很小的epsilon来比较浮点数
        if (fabs(current_res - target_3d_res) > 0.001f) {
            mem_write_float(resolution_3d_address, target_3d_res);
            printf("[3D分辨率锁定#%d] 修正 %.2f → %.2f\n", ++check_count, current_res, target_3d_res);
        } else if (check_count % 10 == 0) {
            printf("[3D分辨率锁定#%d] 正常 %.2f\n", ++check_count, current_res);
        } else {
            check_count++;
        }
        sleep(lock_check_interval_res);
    }
    return NULL;
}

// 显示菜单
void show_menu() {
    // 实时读取当前值
    int current_fps = read_current_fps();
    float current_res = read_current_3d_res();

    printf("\n");
    printf("=================================================================\n");
    printf("   FortniteTools by.妙小橙 QQ群：916981590 免费工具，随缘更新，禁止倒卖，如果你花钱买的，恭喜🎉你被骗了\n");
    printf("=================================================================\n");
    printf("帧率状态:\n");
    printf("   当前帧率:   %d FPS\n", current_fps);
    printf("   目标帧率:   %d FPS\n", target_fps);
    printf("   锁定状态:   %s (检测间隔: %d秒)\n",
           lock_fps_enabled ? "✅ 开启" : "❌ 关闭", lock_check_interval_fps);
    printf("-----------------------------------------------------------------\n");
    printf("3D分辨率状态:\n");
    printf("   当前分辨率: %.2f %%\n", current_res);
    printf("   目标分辨率: %.2f %%\n", target_3d_res);
    printf("   锁定状态:   %s (检测间隔: %d秒)\n",
           lock_res_enabled ? "✅ 开启" : "❌ 关闭", lock_check_interval_res);
    printf("=================================================================\n");
    printf("1. 设置目标帧率\n");
    printf("2. %s帧率锁定\n", lock_fps_enabled ? "禁用" : "启用");
    printf("3. 设置帧率检测间隔\n");
    printf("4. 设置目标3D分辨率\n");
    printf("5. %s3D分辨率锁定\n", lock_res_enabled ? "禁用" : "启用");
    printf("6. 设置3D分辨率检测间隔\n");
    printf("7. 退出程序\n");
    printf("=================================================================\n");
    printf("请选择操作: ");
}

// 清除输入缓冲区
void clear_input() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// 密码验证
int password_check() {
    char input[100];
    const char *correct = "mxc";

    printf("\n========================================\n");
    printf("   密码验证 (密码获取 Q群:916981590)\n");
    printf("========================================\n");
    printf("请输入密码: ");

    if (scanf("%99s", input) != 1) {
        printf("读取失败\n");
        return 0;
    }

    if (strcmp(input, correct) == 0) {
        printf("✅ 验证成功\n");
        return 1;
    } else {
        printf("❌ 密码错误\n");
        return 0;
    }
}

// 修改点2: 重写初始化工具函数，使用新的3D分辨率地址计算方法
int init_tool() {
    int pid = getPID("com.epicgames.fortnite");
    if (pid <= 0) {
        printf("❌ 未找到游戏进程\n");
        return 0;
    }

    char mem_path[64];
    sprintf(mem_path, "/proc/%d/mem", pid);
    handle = open(mem_path, O_RDWR);
    if (handle <= 0) {
        printf("❌ 无法访问内存 (需要Root权限)\n");
        return 0;
    }

    // 获取帧率功能基址
    long int base_cbbss = get_module_base_cbbss(pid); // 用于帧率
    if (base_cbbss == 0) {
        printf("❌ 未找到帧率功能基址 (libUnreal.so:cbbss)\n");
        close(handle);
        handle = 0;
        return 0;
    }

    // 修改点3: 使用新的计算方法
    // 帧率地址 = libUnreal.so:cbbss + 0xFD8D8
    fps_address = base_cbbss + 0xFD8D8;
    
    // 修改点4: 使用3d.cpp的算法计算3D分辨率地址
    // 步骤1: 计算第一级指针地址
    long int first_level_offset = 0x169BC0;
    long int pointer_addr = base_cbbss + first_level_offset;
    
    // 步骤2: 读取第一级指针地址中存储的值
    long int pointer_value = mem_read_long(pointer_addr);
    if (pointer_value == 0) {
        printf("❌ 读取3D分辨率指针值失败\n");
        close(handle);
        handle = 0;
        return 0;
    }
    
    // 步骤3: 计算最终3D分辨率地址
    resolution_3d_address = pointer_value;
    
    // 验证地址是否为有效正数
    if (resolution_3d_address <= 0) {
        printf("❌ 计算得到的3D分辨率地址无效: 0x%lx\n", resolution_3d_address);
        close(handle);
        handle = 0;
        return 0;
    }

    printf("✅ 工具初始化成功 (PID: %d)\n", pid);
    //printf("   - 帧率功能基址 (cbbss): 0x%lx\n", base_cbbss);
    //printf("   - 游戏帧率地址: 0x%lx (0x%lx + 0xFD4D0)\n", fps_address, base_cbbss);
    //printf("   - 3D分辨率地址: 0x%lx\n", resolution_3d_address);

    // 预读一次当前值用于显示初始化
    current_3d_res = read_current_3d_res();
    target_3d_res = current_3d_res;// 初始化时目标值设为当前值
    return 1;
}

// 清理资源
void cleanup() {
    // 停止帧率锁定线程
    if (lock_fps_enabled) {
        lock_fps_enabled = 0;
        pthread_join(lock_fps_thread_tid, NULL);
    }
    // 停止3D分辨率锁定线程
    if (lock_res_enabled) {
        lock_res_enabled = 0;
        pthread_join(lock_res_thread_tid, NULL);
    }
    if (handle > 0) {
        close(handle);
        handle = 0;
    }
}

int main() {
    // 密码验证
    if (!password_check()) {
        printf("程序退出\n");
        return 1;
    }
    clear_input();

    // 初始化
    if (!init_tool()) {
        return 1;
    }

    int choice;
    while (1) {
        show_menu();

        if (scanf("%d", &choice) != 1) {
            printf("❌ 输入错误，请输入数字\n");
            clear_input();
            continue;
        }
        clear_input();

        switch (choice) {
            case 1: {// 设置目标帧率
                printf("请输入目标帧率 (建议范围 30-360): ");
                int new_fps;
                if (scanf("%d", &new_fps) != 1 || new_fps < 30 || new_fps > 360) {
                    printf("❌ 无效输入，请输入30-360之间的整数\n");
                    clear_input();
                    break;
                }
                clear_input();

                target_fps = new_fps;
                if (mem_write_int(fps_address, target_fps) == 0) {
                    printf("✅ 目标帧率已设置为 %d FPS\n", target_fps);
                    int verify_fps = read_current_fps();
                    printf("✅ 验证成功，当前帧率: %d FPS\n", verify_fps);
                } else {
                    printf("❌ 设置失败\n");
                }
                break;
            }

            case 2: {// 切换帧率锁定
                if (lock_fps_enabled) {
                    lock_fps_enabled = 0;
                    pthread_join(lock_fps_thread_tid, NULL);
                    printf("✅ 已禁用帧率锁定\n");
                } else {
                    if (target_fps == 0) {
                        printf("❌ 请先设置目标帧率\n");
                        break;
                    }
                    lock_fps_enabled = 1;
                    pthread_create(&lock_fps_thread_tid, NULL, lock_fps_thread_func, NULL);
                    printf("✅ 已启用帧率锁定\n");
                    printf("   检测间隔: %d秒\n", lock_check_interval_fps);
                }
                break;
            }

            case 3: {// 设置帧率检测间隔
                printf("当前帧率检测间隔: %d秒\n", lock_check_interval_fps);
                printf("请输入新的检测间隔(秒, 建议5-300): ");
                int interval;
                if (scanf("%d", &interval) != 1 || interval < 5 || interval > 300) {
                    printf("❌ 无效输入，请输入5-300之间的整数\n");
                    clear_input();
                    break;
                }
                clear_input();
                lock_check_interval_fps = interval;
                printf("✅ 帧率检测间隔已设为 %d秒\n", interval);
                break;
            }

            case 4: {// 设置目标3D分辨率
                printf("当前3D分辨率: %.2f %%\n", read_current_3d_res());
                printf("请输入目标3D分辨率 (例如: 100.0 表示100%%，50.0表示50%%): ");
                float new_res;
                if (scanf("%f", &new_res) != 1) {
                    printf("❌ 无效输入，请输入一个数字\n");
                    clear_input();
                    break;
                }
                clear_input();

                if (new_res <= 0.0f || new_res > 500.0f) {// 设定一个合理范围
                    printf("⚠️  警告: 输入的值 %.2f 可能超出合理范围，是否继续? (y/n): ", new_res);
                    char confirm;
                    scanf("%c", &confirm);
                    if (confirm != 'y' && confirm != 'Y') {
                        printf("操作已取消\n");
                        clear_input();
                        break;
                    }
                    clear_input();
                }

                target_3d_res = new_res;
                if (mem_write_float(resolution_3d_address, target_3d_res) == 0) {
                    printf("✅ 目标3D分辨率已设置为 %.2f %%\n", target_3d_res);
                    float verify_res = read_current_3d_res();
                    printf("✅ 验证成功，当前3D分辨率: %.2f %%\n", verify_res);
                } else {
                    printf("❌ 设置失败\n");
                }
                break;
            }

            case 5: {// 切换3D分辨率锁定
                if (lock_res_enabled) {
                    lock_res_enabled = 0;
                    pthread_join(lock_res_thread_tid, NULL);
                    printf("✅ 已禁用3D分辨率锁定\n");
                } else {
                    lock_res_enabled = 1;
                    pthread_create(&lock_res_thread_tid, NULL, lock_res_thread_func, NULL);
                    printf("✅ 已启用3D分辨率锁定\n");
                    printf("   检测间隔: %d秒\n", lock_check_interval_res);
                }
                break;
            }

            case 6: {// 设置3D分辨率检测间隔
                printf("当前3D分辨率检测间隔: %d秒\n", lock_check_interval_res);
                printf("请输入新的检测间隔(秒, 建议5-300): ");
                int interval;
                if (scanf("%d", &interval) != 1 || interval < 5 || interval > 300) {
                    printf("❌ 无效输入，请输入5-300之间的整数\n");
                    clear_input();
                    break;
                }
                clear_input();
                lock_check_interval_res = interval;
                printf("✅ 3D分辨率检测间隔已设为 %d秒\n", interval);
                break;
            }

            case 7: {// 退出程序
                printf("\n感谢使用，正在清理资源并退出...\n");
                cleanup();
                printf("再见！\n");
                return 0;
            }

            default: {
                printf("❌ 无效选项，请重新选择\n");
                break;
            }
        }

        sleep(1);// 菜单显示间隔
    }
}
