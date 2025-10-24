#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "cmd.h"
#include "ff.h"
#ifdef _WIN32
#include <direct.h>
#endif

int shell_do_help(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("可用命令:\n");
    printf("  ls [-p] <dir>                 - 列出当前目录内容\n");
    printf("  cd <dir>                      - 切换目录\n");
    printf("  pwd                           - 显示当前目录\n");
    printf("  mkdir [-p] <dir1> <dir2> ...  - 创建目录\n");
    printf("  rm <file>                     - 删除文件\n");
    printf("  read <file> [bytes]           - 读取文件\n");
    printf("  write <file> <data>           - 写入文件\n");
    printf("  stat <path>                   - 检查文件/目录是否存在\n");
    printf("  mv <old> <new>                - 重命名/移动文件或目录\n");
    printf("  chmod <path> <attr>           - 更改文件/目录属性\n");
    printf("  getfree [drive]               - 获取卷空闲空间\n");
    printf("  getlabel <drive>              - 获取卷标\n");
    printf("  setlabel <drive> <label>      - 设置卷标\n");
    printf("  export <src> <dst>            - 导出文件/目录到宿主机\n");
    printf("  clear                         - 清空屏幕\n");
    printf("  help                          - 显示此帮助信息\n");
    printf("  exit                          - 退出shell\n");
    return 0;
}

int shell_do_ls(int argc, char **argv)
{
    DIR     dir;
    FILINFO fno;
    FRESULT fr;
    char   *path        = ".";
    int     show_hidden = 0;
    if (argc > 0) {
        if (strcmp(argv[0], "-a") == 0) {
            show_hidden = 1;
            argc--;
            argv++;
        }
        if (argc == 1) {
            path = argv[argc - 1];
        } else if (argc > 1) {
            fprintf(stderr, "用法: ls [-a] [目录]\n");
            return -1;
        }
    }

    fr = f_opendir(&dir, path);
    if (fr != FR_OK) {
        fprintf(stderr, "无法打开目录: %s\n", path);
        return -1;
    }

    int           entry_num  = 0;
    int           file_count = 0;
    int           dir_count  = 0;
    unsigned long total_size = 0;

    while (1) {
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK || fno.fname[0] == 0)
            break;
        /* 跳过当前目录和父目录项 */
        if (strcmp(fno.fname, ".") == 0 || strcmp(fno.fname, "..") == 0)
            continue;
        // 跳过隐藏文件
        if (!show_hidden && (fno.fattrib & AM_HID || fno.fname[0] == '.'))
            continue;

        // 解析日期和时间
        unsigned short date = fno.fdate;
        unsigned short time = fno.ftime;

        // 从FAT文件系统日期格式转换为可读格式
        int year  = ((date >> 9) & 0x7F) + 1980;
        int month = (date >> 5) & 0x0F;
        int day   = date & 0x1F;

        // 从FAT文件系统时间格式转换为可读格式
        int hour   = (time >> 11) & 0x1F;
        int minute = (time >> 5) & 0x3F;
        // int second = (time & 0x1F) * 2; // FAT存储的是2秒的倍数

        // 格式化输出
        if (fno.fattrib & AM_DIR) {  // 目录
            printf("%04d/%02d/%02d %02d:%02d  %6s %3s  %5s/\n", year, month, day, hour, minute,
                   "<DIR>", "", fno.fname);
            dir_count++;
        } else {  // 文件
            printf("%04d/%02d/%02d %02d:%02d  %6s %3luB  %5s\n", year, month, day, hour, minute,
                   "<FILE>", (unsigned long)fno.fsize, fno.fname);
            file_count++;
            total_size += fno.fsize;
        }
        entry_num++;
    }

    f_closedir(&dir);

    printf("%d 目录, %d 文件, 共 %lu 字节\n", dir_count, file_count, total_size);

    return 0;
}

int shell_do_mkdir(int argc, char **argv)
{
    int create_parent = 0;
    if (argc < 1) {
        fprintf(stderr, "用法: mkdir [-p] <目录1> <目录2> ...\n");
        return -1;
    }
    if (strcmp(argv[0], "-p") == 0) {
        create_parent = 1;
        argc--;
        argv++;
    }

    char cwd[256] = {0};
    if (create_parent) {
        FRESULT fr = f_getcwd(cwd, sizeof(cwd));
        if (fr != FR_OK) {
            return -1;
        }
    }

    for (int i = 0; i < argc; i++) {
        FRESULT fr = FR_OK;
        if (!create_parent) {
            fr = f_mkdir(argv[i]);
        } else {
            char *dir = strtok(argv[i], "/");
            while (dir) {
                fr = f_mkdir(dir);
                if (fr != FR_OK && fr != FR_EXIST) {
                    break;
                }
                fr = f_chdir(dir);
                if (fr != FR_OK && fr != FR_EXIST) {
                    break;
                }
                dir = strtok(NULL, "/");
            }
            fr = f_chdir(cwd);
            if (fr != FR_OK && fr != FR_EXIST) {
                break;
            }
        }
        if (fr != FR_OK && fr != FR_EXIST) {
            fprintf(stderr, "创建目录失败: %s (错误代码: %d)\n", argv[i], fr);
            return -1;
        }
    }

    return 0;
}

int shell_do_rm(int argc, char **argv)
{
    if (argc != 1) {
        fprintf(stderr, "用法: rm <文件名>\n");
        return -1;
    }

    FRESULT fr = f_unlink(argv[0]);
    if (fr != FR_OK) {
        fprintf(stderr, "删除文件/目录失败: %s (错误代码: %d)\n", argv[0], fr);
        return -1;
    }

    return 0;
}

int shell_do_cd(int argc, char **argv)
{
    if (argc != 1) {
        fprintf(stderr, "用法: cd <目录名>\n");
        return -1;
    }

    FRESULT fr = f_chdir(argv[0]);
    if (fr != FR_OK) {
        fprintf(stderr, "切换目录失败: %s (错误代码: %d)\n", argv[0], fr);
        return -1;
    }

    // // 更新全局路径信息
    // fs_context_t *ctx = cmd_args_cast(arg, fs_context_t);
    // if (ctx) {
    //     strcpy(ctx->disk_path, argv[0]);
    //     printf("当前目录: %s\n", ctx->disk_path);
    // }

    return 0;
}

int shell_do_pwd(int argc, char **argv)
{
    if (argc != 0) {
        fprintf(stderr, "用法: pwd\n");
        return -1;
    }

    char    cwd[256] = {0};
    FRESULT fr       = f_getcwd(cwd, sizeof(cwd));
    if (fr != FR_OK) {
        fprintf(stderr, "获取当前目录失败 (错误代码: %d)\n", fr);
        return -1;
    }

    printf("%s\n", cwd);
    return 0;
}

int shell_do_touch(int argc, char **argv)
{
    if (argc != 1) {
        fprintf(stderr, "用法: touch <文件名>\n");
        return -1;
    }

    FIL *fp = (FIL *)malloc(sizeof(FIL));
    if (!fp) {
        return -1;
    }

    FRESULT fr = f_open(fp, argv[0], FA_CREATE_NEW | FA_WRITE);
    if (fr != FR_OK && fr != FR_EXIST) {
        fprintf(stderr, "创建文件失败: %s (错误代码: %d)\n", argv[0], fr);
        free(fp);
        return -1;
    } else if (fr == FR_EXIST) {
        // 使用当前时间作为默认时间戳
        FILINFO fno;
        FRESULT res = f_utime(argv[0], &fno);
        if (res != FR_OK) {
            fprintf(stderr, "更改时间戳失败: %s\n", argv[0]);
            return -1;
        }
    }

    f_close(fp);
    free(fp);

    return 0;
}

int shell_do_mv(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "用法: mv <old_path> <new_path>\n");
        return -1;
    }

    FRESULT res = f_rename(argv[0], argv[1]);
    if (res != FR_OK) {
        fprintf(stderr, "重命名/移动文件失败: %s -> %s\n", argv[0], argv[1]);
        return -1;
    }

    printf("成功重命名/移动: %s -> %s\n", argv[0], argv[1]);
    return 0;
}

int shell_do_read(int argc, char **argv)
{
    if (argc < 1 || argc > 2 || (argc == 2 && atoi(argv[1]) <= 0)) {
        fprintf(stderr, "用法: read <文件名> [字节数]\n");
        return -1;
    }

    const char *filename      = argv[0];
    UINT        bytes_to_read = 0;
    int         read_all      = (argc == 1);  // 如果没有提供字节数参数，则读取整个文件

    FIL     fil;
    FRESULT fr = f_open(&fil, filename, FA_READ);
    if (fr != FR_OK) {
        fprintf(stderr, "打开文件失败: %s (错误代码: %d)\n", filename, fr);
        return -1;
    }

    // 如果没有提供字节数参数，则读取整个文件
    if (read_all) {
        bytes_to_read = (UINT)fil.obj.objsize;  // 获取文件大小
        if (bytes_to_read == 0) {
            printf("文件为空\n");
            f_close(&fil);
            return 0;
        }
    } else {
        bytes_to_read = (UINT)atoi(argv[1]);
    }

    void *buffer = malloc(bytes_to_read);
    if (!buffer) {
        fprintf(stderr, "内存分配失败\n");
        f_close(&fil);
        return -1;
    }

    UINT bytes_read;
    fr = f_read(&fil, buffer, bytes_to_read, &bytes_read);
    if (fr != FR_OK) {
        fprintf(stderr, "读取文件失败: %s (错误代码: %d)\n", filename, fr);
        free(buffer);
        f_close(&fil);
        return -1;
    }

    printf("读取了 %u 字节的数据:", bytes_read);
    // 以十六进制格式显示数据
    for (UINT i = 0; i < bytes_read; i++) {
        if (i % 16 == 0)
            printf("\n%04X: ", i);
        printf("%02X ", ((unsigned char *)buffer)[i]);
    }
    printf("\n");

    free(buffer);
    f_close(&fil);
    return 0;
}

int shell_do_write(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "用法: write <文件名> <数据>\n");
        return -1;
    }

    const char *filename = argv[0];
    // 将所有剩余的参数连接成一个字符串作为数据
    char data[256] = {0};
    int  data_len  = 0;

    for (int i = 1; i < argc; i++) {
        if (i > 1) {
            // 在参数之间添加空格
            data[data_len++] = ' ';
        }
        int arg_len = strlen(argv[i]);
        if (data_len + arg_len < sizeof(data) - 1) {
            strcpy(data + data_len, argv[i]);
            data_len += arg_len;
        } else {
            // 数据太长，截断
            strncpy(data + data_len, argv[i], sizeof(data) - data_len - 1);
            data_len = sizeof(data) - 1;
            printf("数据超过256字节, 超出部分被截断\n");
            break;
        }
    }

    FIL     fil;
    FRESULT fr = f_open(&fil, filename, FA_WRITE | FA_OPEN_ALWAYS);
    if (fr != FR_OK) {
        fprintf(stderr, "打开文件失败: %s (错误代码: %d)\n", filename, fr);
        return -1;
    }

    // 移动到文件末尾
    fr = f_lseek(&fil, f_size(&fil));
    if (fr != FR_OK) {
        fprintf(stderr, "定位文件指针失败 (错误代码: %d)\n", fr);
        f_close(&fil);
        return -1;
    }

    UINT bytes_written;
    fr = f_write(&fil, data, data_len, &bytes_written);
    if (fr != FR_OK) {
        fprintf(stderr, "写入文件失败: %s (错误代码: %d)\n", filename, fr);
        f_close(&fil);
        return -1;
    }

    printf("附加了 %u 字节的数据到文件: %s\n", bytes_written, filename);

    f_close(&fil);
    return 0;
}

int shell_do_stat(int argc, char **argv)
{
    if (argc < 1) {
        fprintf(stderr, "用法: stat <path>\n");
        return -1;
    }

    FILINFO fno;
    FRESULT res = f_stat(argv[0], &fno);
    if (res != FR_OK) {
        fprintf(stderr, "获取文件/目录信息失败: %s\n", argv[0]);
        return -1;
    }

    if (fno.fattrib & AM_DIR) {
        printf("%8s: %s\n", "Dir", fno.fname);
    } else {
        printf("%8s: %s\n", "File", fno.fname);
    }
    // 其他文件属性
    if (fno.fattrib & AM_RDO || fno.fattrib & AM_HID || fno.fattrib & AM_SYS ||
        fno.fattrib & AM_ARC) {
        printf("%8s: ", "Attri");
        if (fno.fattrib & AM_RDO)
            printf("R");
        if (fno.fattrib & AM_HID)
            printf("H");
        if (fno.fattrib & AM_SYS)
            printf("S");
        if (fno.fattrib & AM_ARC)
            printf("A");
        printf("\n");
    }
    // 如果是文件，显示大小（目录的fsize没有被赋值）
    if (!(fno.fattrib & AM_DIR)) {
        printf("%8s: %llu Bytes, %lu Sectors\n", "Size", (unsigned long long)fno.fsize,
               ((fno.fsize + SECTOR_SIZE - 1) / SECTOR_SIZE));
    }
    // 格式化日期和时间
    WORD year   = (fno.fdate >> 9) + 1980;
    WORD month  = (fno.fdate >> 5) & 0x0F;
    WORD day    = fno.fdate & 0x1F;
    WORD hour   = (fno.ftime >> 11);
    WORD minute = (fno.ftime >> 5) & 0x3F;
    WORD second = (fno.ftime & 0x1F) * 2;
    printf("%8s: %04d-%02d-%02d %02d:%02d:%02d\n", fno.fattrib & AM_DIR ? "Created" : "Modified",
           year, month, day, hour, minute, second);

    return 0;
}

int shell_do_chmod(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "用法: chmod <path> <attr>\n");
        return -1;
    }

    // 解析属性参数
    BYTE attr = 0;
    if (strstr(argv[1], "R"))
        attr |= AM_RDO;
    if (strstr(argv[1], "H"))
        attr |= AM_HID;
    if (strstr(argv[1], "S"))
        attr |= AM_SYS;

    FRESULT res = f_chmod(argv[0], attr, AM_RDO | AM_HID | AM_SYS);
    if (res != FR_OK) {
        fprintf(stderr, "更改属性失败: %s\n", argv[0]);
        return -1;
    }

    printf("成功更改属性: %s\n", argv[0]);
    return 0;
}

// int shell_do_chdrive(int argc, char **argv)
// {
//     if (argc < 1) {
//         fprintf(stderr, "用法: chdrive <drive>\n");
//         return -1;
//     }

//     // 在这个简化实现中，我们假设只有一个驱动器
//     printf("驱动器已切换到: %s\n", argv[0]);
//     return 0;
// }

// int shell_do_mount(int argc, char **argv)
// {
//     if (argc < 2) {
//         fprintf(stderr, "用法: mount <drive> <path>\n");
//         return -1;
//     }

//     // 在实际应用中，这里会挂载文件系统
//     // 但在我们的简化实现中，我们只是模拟这个操作
//     printf("成功挂载驱动器 %s 到路径 %s\n", argv[0], argv[1]);
//     return 0;
// }

// int shell_do_mkfs(int argc, char **argv)
// {
//     if (argc < 1) {
//         fprintf(stderr, "用法: mkfs <drive>\n");
//         return -1;
//     }

//     // 在实际应用中，这里会创建FAT卷
//     // 但在我们的简化实现中，我们只是模拟这个操作
//     printf("成功在驱动器 %s 上创建FAT卷\n", argv[0]);
//     return 0;
// }

// int shell_do_fdisk(int argc, char **argv)
// {
//     if (argc < 1) {
//         fprintf(stderr, "用法: fdisk <drive>\n");
//         return -1;
//     }

//     // 在实际应用中，这里会在物理驱动器上创建分区
//     // 但在我们的简化实现中，我们只是模拟这个操作
//     printf("成功在驱动器 %s 上创建分区\n", argv[0]);
//     return 0;
// }

int shell_do_getfree(int argc, char **argv)
{
    DWORD  fre_clust, fre_sect, tot_sect;
    FATFS *fs;

    // 如果没有参数，则显示所有可能驱动器的信息
    if (argc < 1) {
        printf("驱动器列表 (最大支持 %d 个驱动器):\n", FF_VOLUMES);

        int mounted_count = 0;
        // 遍历所有可能的驱动器（根据FF_VOLUMES配置）
        for (int i = 0; i < FF_VOLUMES; i++) {
            // 构造驱动器路径
            char drive_path[4] = {0};
            if (FF_STR_VOLUME_ID == 0) {
                drive_path[0] = '0' + i;  // 数字驱动器号
            }

            // 尝试获取驱动器的空闲空间信息
            FRESULT res = f_getfree(drive_path, &fre_clust, &fs);
            if (res == FR_OK) {
                mounted_count++;
                // 计算扇区数量
                tot_sect = (fs->n_fatent - 2) * fs->csize;
                fre_sect = fre_clust * fs->csize;

                printf("  驱动器 %d:\n", i);
                printf("    总扇区数: %lu\n", (unsigned long)tot_sect);
                printf("    空闲扇区数: %lu\n", (unsigned long)fre_sect);
                printf("    簇大小: %u 扇区\n", (unsigned int)fs->csize);
                printf("    文件系统类型: ");
                switch (fs->fs_type) {
                    case FS_FAT12:
                        printf("FAT12");
                        break;
                    case FS_FAT16:
                        printf("FAT16");
                        break;
                    case FS_FAT32:
                        printf("FAT32");
                        break;
                    case FS_EXFAT:
                        printf("exFAT");
                        break;
                    default:
                        printf("未知");
                        break;
                }
                printf("\n");
            } else if (res == FR_INVALID_DRIVE) {
                printf("  驱动器 %d: 未挂载或无效\n", i);
            } else {
                printf("  驱动器 %d: 获取信息失败 (错误码: %d)\n", i, res);
            }
        }

        if (mounted_count == 0) {
            printf("  没有已挂载的驱动器\n");
        }

        return 0;
    }

    // 如果提供了驱动器参数，则显示指定驱动器的信息
    FRESULT res = f_getfree(argv[0], &fre_clust, &fs);
    if (res != FR_OK) {
        fprintf(stderr, "获取空闲空间失败: %s (错误码: %d)\n", argv[0], res);
        return -1;
    }

    // 计算扇区数量
    tot_sect = (fs->n_fatent - 2) * fs->csize;
    fre_sect = fre_clust * fs->csize;

    printf("驱动器 %s 的空闲空间:\n", argv[0]);
    printf("  总扇区数: %lu\n", (unsigned long)tot_sect);
    printf("  空闲扇区数: %lu\n", (unsigned long)fre_sect);
    printf("  簇大小: %u 扇区\n", (unsigned int)fs->csize);

    return 0;
}

int shell_do_getlabel(int argc, char **argv)
{
    if (argc < 1) {
        fprintf(stderr, "用法: getlabel <驱动器>\n");
        return -1;
    }

    char  label[12];
    DWORD vsn;

    FRESULT res = f_getlabel(argv[0], label, &vsn);
    if (res != FR_OK) {
        fprintf(stderr, "获取卷标失败: %s\n", argv[0]);
        return -1;
    }

    printf("驱动器 %s 的卷标: %s\n", argv[0], label);
    printf("卷序列号: %08lX\n", (unsigned long)vsn);

    return 0;
}

int shell_do_setlabel(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "用法: setlabel <驱动器> <卷标>\n");
        return -1;
    }

    FRESULT res = f_setlabel(argv[1]);
    if (res != FR_OK) {
        fprintf(stderr, "设置卷标失败: %s\n", argv[1]);
        return -1;
    }

    return 0;
}

int shell_do_export(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "用法: export <源路径> <目标路径>\n");
        return -1;
    }

    const char *src_path = argv[0];  // 文件系统中的源路径
    const char *dst_path = argv[1];  // 宿主机文件系统的目标路径

    // 检查源路径是否存在
    FILINFO fno;
    FRESULT res = f_stat(src_path, &fno);
    if (res != FR_OK) {
        fprintf(stderr, "源路径不存在: %s\n", src_path);
        return -1;
    }

    // 如果是目录，递归导出
    if (fno.fattrib & AM_DIR) {
        // 创建目标目录
#ifdef _WIN32
        _mkdir(dst_path);
#else
        mkdir(dst_path, 0755);
#endif

        // 打开目录
        DIR dir;
        res = f_opendir(&dir, src_path);
        if (res != FR_OK) {
            fprintf(stderr, "无法打开目录: %s\n", src_path);
            return -1;
        }

        // 遍历目录中的所有文件和子目录
        while (1) {
            res = f_readdir(&dir, &fno);
            if (res != FR_OK || fno.fname[0] == 0)
                break;

            // 跳过当前目录和父目录项
            if (strcmp(fno.fname, ".") == 0 || strcmp(fno.fname, "..") == 0)
                continue;

            // 构造源路径和目标路径
            char full_src_path[256];
            char full_dst_path[256];

            if (strcmp(src_path, "/") == 0) {
                snprintf(full_src_path, sizeof(full_src_path), "/%s", fno.fname);
            } else {
                snprintf(full_src_path, sizeof(full_src_path), "%s/%s", src_path, fno.fname);
            }

            snprintf(full_dst_path, sizeof(full_dst_path), "%s/%s", dst_path, fno.fname);

            // 递归导出
            char *sub_argv[] = {full_src_path, full_dst_path};
            shell_do_export(2, sub_argv);
        }

        f_closedir(&dir);
    } else {
        // 如果是文件，复制文件内容
        FIL   src_file;
        FILE *dst_file;

        // 打开源文件（FAT文件系统中的文件）
        res = f_open(&src_file, src_path, FA_READ);
        if (res != FR_OK) {
            fprintf(stderr, "无法打开源文件: %s\n", src_path);
            return -1;
        }

        // 创建目标文件（宿主机文件系统中的文件）
        dst_file = fopen(dst_path, "wb");
        if (!dst_file) {
            fprintf(stderr, "无法创建目标文件: %s\n", dst_path);
            f_close(&src_file);
            return -1;
        }

        // 复制文件内容
        char   buffer[1024];
        UINT   bytes_read;
        size_t bytes_written;

        while (1) {
            res = f_read(&src_file, buffer, sizeof(buffer), &bytes_read);
            if (res != FR_OK || bytes_read == 0)
                break;

            bytes_written = fwrite(buffer, 1, bytes_read, dst_file);
            if (bytes_written != bytes_read) {
                fprintf(stderr, "写入目标文件时出错: %s\n", dst_path);
                f_close(&src_file);
                fclose(dst_file);
                return -1;
            }
        }

        // 关闭文件
        f_close(&src_file);
        fclose(dst_file);

        printf("成功导出文件: %s -> %s\n", src_path, dst_path);
    }

    return 0;
}

// int shell_do_setcp(int argc, char **argv)
// {
//     if (argc < 1) {
//         fprintf(stderr, "用法: setcp <codepage>\n");
//         return -1;
//     }

//     // FRESULT res = f_setcp(atoi(argv[0]));
//     // if (res != FR_OK) {
//     //     fprintf(stderr, "设置活动代码页失败: %s\n", argv[0]);
//     //     return -1;
//     // }

//     // 在实际应用中，这里会设置活动代码页
//     // 但在我们的简化实现中，我们只是模拟这个操作
//     printf("成功设置活动代码页为: %s\n", argv[0]);
//     return 0;
// }

#define SHELL_PROMPT "fatfs> "
#define SHELL_MAX_CMD_ARGS 10

// int shell_fprintf(FILE* stream, const char *fmt, ...)
// {
//     int n = 0;
//     va_list args;
//     va_start(args, fmt);
//     fprintf(stream, SHELL_PROMPT);
//     n = vfprintf(stream, fmt, args);
//     va_end(args);
//     return n;
// }

// int shell_printf(const char *fmt, ...)
// {
//     int n = 0;
//     va_list args;
//     va_start(args, fmt);
//     n = vprintf(fmt, args);
//     va_end(args);
//     return n;
// }

// 解析命令行参数，支持引号内的空格
int parse_args(char *input, char *argv[], int max_args)
{
    int   argc = 0;
    char *p    = input;

    while (*p && argc < max_args) {
        // 跳过空格
        while (*p == ' ')
            p++;
        if (!*p)
            break;

        // 处理引号内的参数
        if (*p == '"' || *p == '\'') {
            char quote = *p;
            p++;  // 跳过开始引号
            argv[argc++] = p;

            // 查找结束引号
            while (*p && *p != quote)
                p++;
            if (*p) {
                *p = '\0';  // 用空字符替换结束引号
                p++;
            }
        } else {
            // 处理无引号的参数
            argv[argc++] = p;

            // 查找参数结束位置
            while (*p && *p != ' ')
                p++;
            if (*p) {
                *p = '\0';  // 用空字符替换空格
                p++;
            }
        }
    }

    return argc;
}

int shell_run(void)
{
    char  input[256]               = {0};
    char *argv[SHELL_MAX_CMD_ARGS] = {0};
    int   argc                     = 0;
    int   ret                      = 0;

    printf("FatFS Shell - 输入 'help' 查看可用命令，'exit' 退出\n");

    while (1) {
        printf("%s", SHELL_PROMPT);
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }

        // 移除换行符
        input[strcspn(input, "\n")] = 0;

        // 空命令处理
        if (strlen(input) == 0) {
            continue;
        }

        // 解析命令行参数
        argc = parse_args(input, argv, SHELL_MAX_CMD_ARGS);

#define _shell_cmd0_is(cmd_name) (strcmp(argv[0], #cmd_name) == 0)
#define _shell_cmd1_is(cmd_name) (_shell_cmd0_is(cmd_name) && argc > 1)

        // 内置命令处理
        if (_shell_cmd0_is(exit)) {
            break;
        } else if (_shell_cmd0_is(help)) {
            ret = shell_do_help(argc, argv);
        } else if (_shell_cmd0_is(clear)) {
#ifdef _WIN32
            system("cls");
#else
            system("clear");
#endif
        } else if (_shell_cmd0_is(ls)) {
            ret = shell_do_ls(argc - 1, argv + 1);
        } else if (_shell_cmd0_is(pwd)) {
            ret = shell_do_pwd(argc - 1, argv + 1);
        } else if (_shell_cmd0_is(mkdir)) {
            ret = shell_do_mkdir(argc - 1, argv + 1);
        } else if (_shell_cmd0_is(rm)) {
            ret = shell_do_rm(argc - 1, argv + 1);
        } else if (_shell_cmd0_is(cd)) {
            ret = shell_do_cd(argc - 1, argv + 1);
        } else if (_shell_cmd0_is(touch)) {
            ret = shell_do_touch(argc - 1, argv + 1);
        } else if (_shell_cmd0_is(read)) {
            ret = shell_do_read(argc - 1, argv + 1);
        } else if (_shell_cmd0_is(write)) {
            ret = shell_do_write(argc - 1, argv + 1);
        } else if (_shell_cmd0_is(stat)) {
            ret = shell_do_stat(argc - 1, argv + 1);
        } else if (_shell_cmd0_is(mv)) {
            ret = shell_do_mv(argc - 1, argv + 1);
        } else if (_shell_cmd0_is(chmod)) {
            ret = shell_do_chmod(argc - 1, argv + 1);
        } else if (_shell_cmd0_is(getfree)) {
            ret = shell_do_getfree(argc - 1, argv + 1);
        } else if (_shell_cmd0_is(getlabel)) {
            ret = shell_do_getlabel(argc - 1, argv + 1);
        } else if (_shell_cmd0_is(setlabel)) {
            ret = shell_do_setlabel(argc - 1, argv + 1);
        } else if (_shell_cmd0_is(export)) {
            ret = shell_do_export(argc - 1, argv + 1);
        } else {
            fprintf(stderr, "未知命令: %s。输入 'help' 查看可用命令。\n", argv[0]);
            ret = -1;
        }

#undef _shell_cmd0_is
#undef _shell_cmd0_is
    }

    return ret;
}