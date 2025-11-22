#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "cmd.h"
#include "ff.h"
#include "fferrno.h"
#ifdef _WIN32
#include <direct.h>
#endif

int shell_do_help(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("可用命令:\n");
    printf("  ls [-a -r] <dir>                       - 列出当前目录内容\n");
    printf("  cd <dir>                               - 切换目录\n");
    printf("  pwd                                    - 显示当前目录\n");
    printf("  mkdir [-p] <dir1> <dir2> ...           - 创建目录\n");
    printf("  rm [-r] <file1> [<file2> ...]          - 删除文件\n");
    printf("  read <file> [bytes]                    - 读取文件\n");
    printf("  write <file> <data>                    - 写入文件\n");
    printf("  head <file> [-n lines]                 - 读取文件前n行\n");
    printf("  truncate <file> [-p pos] [-s bytes]    - 从指定位置截断文件到指定大小\n");
    printf("  stat <path>                            - 检查文件/目录是否存在\n");
    printf("  mv <old> <new>                         - 重命名/移动文件或目录\n");
    printf("  touch <file>                           - 创建文件或更新文件时间戳\n");
    printf("  chmod +/-<attr> [...] <path>           - 更改文件/目录属性\n");
    printf("  getfree [<drive>]                      - 获取卷空闲空间\n");
    printf("  getlabel <drive>                       - 获取卷标\n");
    printf("  setlabel <drive> <label>               - 设置卷标\n");
    printf("  export <src> <dst>                     - 导出文件/目录到宿主机\n");
    printf("  clear                                  - 清空屏幕\n");
    printf("  help                                   - 显示此帮助信息\n");
    printf("  exit                                   - 退出shell\n");
    return 0;
}

static void _print_tree(const TCHAR *path, int level, int show_hidden, int is_last[])
{
    DIR     dp;
    FILINFO fno;
    FRESULT fr;
    int     entry_count   = 0;
    int     current_entry = 0;

    fr = f_opendir(&dp, path);
    if (fr != FR_OK) {
        fprintf(stderr, "无法打开目录: %s\n", path);
        return;
    }

    // First pass: count entries
    while (1) {
        fr = f_readdir(&dp, &fno);
        if (fr != FR_OK || fno.fname[0] == 0)
            break;

        /* 跳过当前目录和父目录项 */
        if (strcmp(fno.fname, ".") == 0 || strcmp(fno.fname, "..") == 0)
            continue;

        // 跳过隐藏文件
        if (!show_hidden && (fno.fattrib & AM_HID || fno.fname[0] == '.'))
            continue;

        entry_count++;
    }

    // Reset directory reading
    f_rewinddir(&dp);
    current_entry = 0;

    while (1) {
        fr = f_readdir(&dp, &fno);
        if (fr != FR_OK || fno.fname[0] == 0)
            break;

        /* 跳过当前目录和父目录项 */
        if (strcmp(fno.fname, ".") == 0 || strcmp(fno.fname, "..") == 0)
            continue;

        // 跳过隐藏文件
        if (!show_hidden && (fno.fattrib & AM_HID || fno.fname[0] == '.'))
            continue;

        current_entry++;

        // 打印缩进
        for (int i = 0; i < level; i++) {
            if (is_last[i])
                // printf("    ");
                printf("  ");
            else
                // printf("|   ");
                printf("│ ");
        }

        // 打印树形连接符
        int is_last_entry = (current_entry == entry_count);
        if (is_last_entry)
            // printf("`-- ");
            printf("└─");
        else
            // printf("|-- ");
            printf("├─");

        if (fno.fattrib & AM_DIR) {
            printf("%s/\n", fno.fname);
            // 递归打印子目录
            TCHAR full_path[256] = {0};
            if (strcmp(path, ".") == 0) {
                snprintf(full_path, sizeof(full_path), "%s", fno.fname);
            } else {
                snprintf(full_path, sizeof(full_path), "%s/%s", path, fno.fname);
            }
            is_last[level] = is_last_entry;
            _print_tree(full_path, level + 1, show_hidden, is_last);
        } else {
            printf("%s\n", fno.fname);
        }
    }

    f_closedir(&dp);
}

int shell_do_ls(int argc, char **argv)
{
    DIR     dp;
    FILINFO fno;
    FRESULT fr;
    TCHAR  *path        = NULL;
    int     show_hidden = 0;
    int     recursive   = 0;

    for (int i = 0; i < argc; ++i) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j] != '\0'; ++j) {
                if (argv[i][j] == 'a') {
                    show_hidden = 1;
                } else if (argv[i][j] == 'r') {
                    recursive = 1;
                } else {
                    fprintf(stderr, "未知选项: -%c, 用法: ls [-a -r] [目录]\n", argv[i][j]);
                    return -1;
                }
            }
        } else {
            if (path) {
                fprintf(stderr, "用法: ls [-a -r] [目录]\n");
                return -1;
            } else {
                path = argv[i];
            }
        }
    }
    if (!path) {
        path = ".";
    }

    if (recursive) {
        printf("%s\n", path);
        int is_last[256] = {0};  // Track if each level is the last entry
        _print_tree(path, 0, show_hidden, is_last);
        return 0;
    }

    fr = f_opendir(&dp, path);
    if (fr != FR_OK) {
        fprintf(stderr, "无法打开目录: %s\n", path);
        return -1;
    }

    int           entry_num  = 0;
    int           file_count = 0;
    int           dir_count  = 0;
    unsigned long total_size = 0;

    while (1) {
        fr = f_readdir(&dp, &fno);
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

    f_closedir(&dp);

    printf("%d 目录, %d 文件, 共 %lu 字节\n", dir_count, file_count, total_size);

    return 0;
}

int shell_do_mkdir(int argc, char **argv)
{
    if (argc < 1) {
        fprintf(stderr, "用法: mkdir [-p] <目录1> <目录2> ...\n");
        return -1;
    }

    int create_parent = 0;
    for (int i = 0; i < argc; ++i) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j] != '\0'; ++j) {
                if (argv[i][j] == 'p') {
                    create_parent = 1;
                } else {
                    fprintf(stderr, "未知选项: -%c, 用法: mkdir [-p] <目录1> <目录2> ...\n",
                            argv[i][j]);
                    return -1;
                }
            }
        }
    }

    TCHAR cwd[256] = {0};
    if (create_parent) {
        FRESULT fr = f_getcwd(cwd, sizeof(cwd));
        if (fr != FR_OK) {
            return -1;
        }
    }

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0)
            continue;
        FRESULT fr = FR_OK;
        if (!create_parent) {
            fr = f_mkdir(argv[i]);
        } else {
            TCHAR *dir = strtok(argv[i], "/");
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
            fprintf(stderr, "创建目录失败: %s (%s: %d)\n", argv[i], f_strerror(fr), fr);
            return -1;
        }
    }

    return 0;
}

static FRESULT _do_unlink(const TCHAR *path, int recursive)
{
    FRESULT fr = FR_OK;

    FILINFO fno;
    fr = f_stat(path, &fno);
    if (fr != FR_OK) {
        return fr;
    }

    if ((fno.fattrib & AM_DIR) && recursive) {
        DIR dp;
        fr = f_opendir(&dp, path);
        if (fr != FR_OK) {
            return fr;
        }
        int is_empty = 1;
        while (1) {
            fr = f_readdir(&dp, &fno);
            if (fr != FR_OK || fno.fname[0] == 0) {
                is_empty = 1;  // 已经删完了所有子目录，原本非空的目录就变成空目录了
                break;
            }
            is_empty = 0;

            // 跳过当前目录和父目录项
            if (strcmp(fno.fname, ".") == 0 || strcmp(fno.fname, "..") == 0)
                continue;

            TCHAR  relpath[256] = {0};
            size_t len          = strlen(path);
            strncpy(relpath, path, len);
            relpath[len] = '/';
            fr           = _do_unlink(strcat(relpath, fno.fname), recursive);
            if (fr != FR_OK) {
                break;
            }
        }
        f_closedir(&dp);
        if (is_empty) {
            fr = f_unlink(path);
        }
    } else {
        fr = f_unlink(path);
    }

    return fr;
}

int shell_do_rm(int argc, char **argv)
{
    if (argc < 1) {
        fprintf(stderr, "用法: rm [-r] <file1> [<file2> ...]\n");
        return -1;
    }

    int recursive = 0;
    for (int i = 0; i < argc; ++i) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j] != '\0'; ++j) {
                if (argv[i][j] == 'r') {
                    recursive = 1;
                } else {
                    fprintf(stderr, "未知选项: -%c, 用法: rm [-r] <file1> [<file2> ...]\n",
                            argv[i][j]);
                    return -1;
                }
            }
        }
    }

    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-r") != 0) {
            FRESULT fr = _do_unlink(argv[i], recursive);
            if (fr != FR_OK) {
                fprintf(stderr, "删除文件/目录失败: %s (%s: %d)\n", argv[0], f_strerror(fr), fr);
                return -1;
            }
        }
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
        fprintf(stderr, "切换目录失败: %s (%s: %d)\n", argv[0], f_strerror(fr), fr);
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

    TCHAR   cwd[256] = {0};
    FRESULT fr       = f_getcwd(cwd, sizeof(cwd));
    if (fr != FR_OK) {
        fprintf(stderr, "获取当前目录失败 (%s: %d)\n", f_strerror(fr), fr);
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
        fprintf(stderr, "创建文件失败: %s (%s: %d)\n", argv[0], f_strerror(fr), fr);
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

    FRESULT fr = f_rename(argv[0], argv[1]);
    if (fr != FR_OK) {
        fprintf(stderr, "重命名/移动文件失败: %s -> %s (%s: %d)\n", argv[0], argv[1],
                f_strerror(fr), fr);
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

    const TCHAR *filename      = argv[0];
    UINT         bytes_to_read = 0;
    int          read_all      = (argc == 1);  // 如果没有提供字节数参数，则读取整个文件

    FIL     fp;
    FRESULT fr = f_open(&fp, filename, FA_READ);
    if (fr != FR_OK) {
        fprintf(stderr, "打开文件失败: %s (%s: %d)\n", filename, f_strerror(fr), fr);
        return -1;
    }

    // 如果没有提供字节数参数，则读取整个文件
    if (read_all) {
        bytes_to_read = (UINT)fp.obj.objsize;  // 获取文件大小
        if (bytes_to_read == 0) {
            printf("文件为空\n");
            f_close(&fp);
            return 0;
        }
    } else {
        bytes_to_read = (UINT)atoi(argv[1]);
    }

    void *buff = malloc(bytes_to_read);
    if (!buff) {
        fprintf(stderr, "内存分配失败\n");
        f_close(&fp);
        return -1;
    }

    UINT bytes_read;
    fr = f_read(&fp, buff, bytes_to_read, &bytes_read);
    if (fr != FR_OK) {
        fprintf(stderr, "读取文件失败: %s (%s: %d)\n", filename, f_strerror(fr), fr);
        free(buff);
        f_close(&fp);
        return -1;
    }

    printf("读取了 %u 字节的数据:", bytes_read);
    // 以十六进制格式显示数据
    for (UINT i = 0; i < bytes_read; i++) {
        if (i % 16 == 0)
            printf("\n%04X: ", i);
        printf("%02X ", ((unsigned char *)buff)[i]);
    }
    printf("\n");

    free(buff);

    f_close(&fp);
    return 0;
}

int shell_do_write(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "用法: write <文件名> <数据>\n");
        return -1;
    }

    const TCHAR *filename = argv[0];
    // 将所有剩余的参数连接成一个字符串作为数据
    TCHAR  data[256] = {0};
    size_t data_len  = 0;

    for (int i = 1; i < argc; i++) {
        if (i > 1) {
            // 在参数之间添加空格
            data[data_len++] = ' ';
        }
        size_t arg_len = strlen(argv[i]);
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

    FIL     fp;
    FRESULT fr = f_open(&fp, filename, FA_WRITE | FA_OPEN_ALWAYS);
    if (fr != FR_OK) {
        fprintf(stderr, "打开文件失败: %s (%s: %d)\n", filename, f_strerror(fr), fr);
        return -1;
    }

    // 移动到文件末尾
    fr = f_lseek(&fp, f_size(&fp));
    if (fr != FR_OK) {
        fprintf(stderr, "定位文件指针失败 (%s: %d)\n", f_strerror(fr), fr);
        f_close(&fp);
        return -1;
    }

    UINT bytes_written;
    fr = f_write(&fp, data, (UINT)data_len, &bytes_written);
    if (fr != FR_OK) {
        fprintf(stderr, "写入文件失败: %s (%s: %d)\n", filename, f_strerror(fr), fr);
        f_close(&fp);
        return -1;
    }

    printf("附加了 %u 字节的数据到文件: %s\n", bytes_written, filename);

    f_close(&fp);
    return 0;
}

int shell_do_head(int argc, char **argv)
{
    int          nlines   = 10;  // 默认10行
    int          found_n  = 0;
    const TCHAR *filename = NULL;

    if (argc < 1 || argc > 3) {
        fprintf(stderr, "用法: head <file> [-n lines]\n");
        return -1;
    }

    for (int i = 0; i < argc; ++i) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j] != '\0'; ++j) {
                if (argv[i][j] == 'n') {
                    nlines = atoi(argv[i + 1]);
                    if (nlines == 0) {
                        fprintf(stderr, "lines must be a positive number\n");
                        return -1;
                    }
                    found_n = 1;
                    if (argc == 3) {
                        filename = argv[i == 0 ? 2 : 0];
                    }
                } else {
                    fprintf(stderr, "未知选项: -%c, 用法: head <file> [-n lines]\n", argv[i][1]);
                    return -1;
                }
            }
        }
    }

    // for (int i = 0; i < argc - 1; ++i) {
    //     if (strcmp(argv[i], "-n") == 0) {
    //         nlines = atoi(argv[i + 1]);
    //         if (nlines == 0) {
    //             fprintf(stderr, "lines must be a positive number\n");
    //             return -1;
    //         }
    //         found_n = 1;
    //         if (argc == 3) {
    //             filename = argv[i == 0 ? 2 : 0];
    //         }
    //         break;
    //     }
    // }
    if ((found_n == 0 && argc != 1) || (found_n == 1 && argc != 3)) {
        fprintf(stderr, "用法: head <file> [-n lines]\n");
        return -1;
    }
    if (found_n == 0 && argc == 1) {
        filename = argv[0];
    }

    FIL     fp;
    FRESULT fr = f_open(&fp, filename, FA_OPEN_EXISTING | FA_READ);
    if (fr != FR_OK) {
        fprintf(stderr, "打开文件失败: %s (%s: %d)\n", filename, f_strerror(fr), fr);
        return -1;
    }

    FILINFO fno;
    fr = f_stat(filename, &fno);
    if (fr != FR_OK) {
        fprintf(stderr, "获取文件信息失败: %s (%s: %d)\n", filename, f_strerror(fr), fr);
        f_close(&fp);
        return -1;
    }

    if (fno.fattrib & AM_DIR) {
        fprintf(stderr, "%s 是一个目录\n", filename);
        f_close(&fp);
        return -1;
    }

    TCHAR *buff = (TCHAR *)calloc(1, fno.fsize);
    if (!buff) {
        fprintf(stderr, "内存不足，无法读取 %s 内容\n", filename);
        f_close(&fp);
        return -1;
    }

    while (nlines--) {
        if (!f_gets(buff, fno.fsize - 1, &fp)) {
            if (f_eof(&fp)) {
                break;
            }
            fprintf(stderr, "读取 %s 失败 (%s: %d)\n", filename, f_strerror(fr), f_error(&fp));
            free(buff);
            f_close(&fp);
            return -1;
        }
        printf("%s\n", buff);
    }

    free(buff);
    f_close(&fp);
    return 0;
}

int shell_do_truncate(int argc, char **argv)
{
    int          nbytes       = -1;
    int          found_nbytes = 0;
    int          pos          = 0;
    int          found_pos    = 0;
    const TCHAR *filename     = NULL;

    for (int i = 0; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j] != '\0'; ++j) {
                if (argv[i][j] == 's') {
                    if (j + 1 >= argc || argv[i][j + 1] != '\0') {
                        fprintf(stderr, "-s 需要参数。用法: truncate <file> [-s bytes] [-p pos]\n");
                        return -1;
                    }
                    nbytes = atoi(argv[i + 1]);
                    if (nbytes < 0) {
                        fprintf(stderr, "bytes must bigger than 0.\n");
                        return -1;
                    }
                    found_nbytes = 1;
                    i++;
                    break;
                } else if (argv[i][j] == 'p') {
                    pos = atoi(argv[i + 1]);
                    if (pos < 0) {
                        if (j + 1 >= argc || argv[i][j + 1] != '\0') {
                            fprintf(stderr,
                                    "-p 需要参数。用法: truncate <file> [-s bytes] [-p pos]\n");
                            return -1;
                        }
                        fprintf(stderr, "pos must bigger than 0.\n");
                        return -1;
                    }
                    found_pos = 1;
                    i++;
                    break;
                } else {
                    fprintf(stderr, "未知选项: -%c, 用法: truncate <file> [-s bytes] [-p pos]\n",
                            argv[i][1]);
                    return -1;
                }
            }
        } else {
            if (filename) {
                fprintf(stderr, "用法: truncate <file> [-s bytes] [-p pos]\n");
                return -1;
            }
            filename = argv[i];
        }

        // if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
        //     nbytes = atoi(argv[i + 1]);
        //     if (nbytes < 0) {
        //         fprintf(stderr, "bytes must bigger than 0.\n");
        //         return -1;
        //     }
        //     found_nbytes = 1;
        //     i++;  // 跳过参数值
        // } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
        //     pos = atoi(argv[i + 1]);
        //     if (pos < 0) {
        //         fprintf(stderr, "pos must bigger than 0.\n");
        //         return -1;
        //     }
        //     found_pos = 1;
        //     i++;  // 跳过参数值
        // } else if (filename == NULL) {
        //     // 第一个非选项参数作为文件名
        //     filename = argv[i];
        // } else {
        //     fprintf(stderr, "用法: truncate <file> [-s bytes] [-p pos]\n");
        //     return -1;
        // }
    }

    if (filename == NULL) {
        fprintf(stderr, "用法: truncate <file> [-s bytes] [-p pos]\n");
        return -1;
    }

    FIL     fp;
    FRESULT fr = f_open(&fp, filename, FA_OPEN_EXISTING | FA_WRITE);
    if (fr != FR_OK) {
        fprintf(stderr, "打开文件失败: %s (%s: %d)\n", filename, f_strerror(fr), fr);
        return -1;
    }

    // 如果指定了位置，则移动文件指针到该位置
    if (found_pos) {
        fr = f_lseek(&fp, pos);
        if (fr != FR_OK) {
            fprintf(stderr, "定位文件指针失败: %s (%s: %d)\n", filename, f_strerror(fr), fr);
            f_close(&fp);
            return -1;
        }
    }

    // 如果指定了大小，则调整文件大小
    if (found_nbytes) {
        // 先扩展文件到指定大小（如果需要）
        fr = f_lseek(&fp, pos + nbytes);
        if (fr != FR_OK) {
            fprintf(stderr, "定位文件指针失败: %s (%s: %d)\n", filename, f_strerror(fr), fr);
            f_close(&fp);
            return -1;
        }

        // f_expend 只能对大小为0B的文件进行扩容
        // fr = f_expand(&fp, nbytes, 1);
        // if (fr != FR_OK) {
        //     fprintf(stderr, "扩展文件失败: %s (%s: %d)\n", filename, fr);
        //     f_close(&fp);
        //     return -1;
        // }
    }

    fr = f_truncate(&fp);
    if (fr != FR_OK) {
        fprintf(stderr, "截断文件失败: %s (%s: %d)\n", filename, f_strerror(fr), fr);
        f_close(&fp);
        return -1;
    }

    f_close(&fp);
    return 0;
}

int shell_do_stat(int argc, char **argv)
{
    if (argc < 1) {
        fprintf(stderr, "用法: stat <path>\n");
        return -1;
    }

    FILINFO fno;
    FRESULT fr = f_stat(argv[0], &fno);
    if (fr != FR_OK) {
        fprintf(stderr, "获取文件/目录信息失败: %s (%s: %d)\n", argv[0], f_strerror(fr), fr);
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
        printf("%8s: %c%c%c%c\n", "Attri", fno.fattrib & AM_RDO ? 'R' : '-',
               fno.fattrib & AM_HID ? 'H' : '-', fno.fattrib & AM_SYS ? 'S' : '-',
               fno.fattrib & AM_ARC ? 'A' : '-');
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

static int _deal_with_attributes(BYTE *attr, const char *attributes, size_t attributes_len,
                                 char opt)
{
    if (!attr || !attributes) {
        return 0;
    }
#define _ADD_ATTRIBUTE(origin, attr) origin |= (attr)
#define _REMOVE_ATTRIBUTE(origin, attr) origin &= ~(attr)
    BYTE attribute = 0;
    for (int i = 0; i < attributes_len; ++i) {
        switch (attributes[i]) {
            case 'R':
                if (opt == '+')
                    _ADD_ATTRIBUTE(attribute, AM_RDO);
                else
                    _REMOVE_ATTRIBUTE(*attr, AM_RDO);
                break;
            case 'H':
                if (opt == '+')
                    _ADD_ATTRIBUTE(attribute, AM_HID);
                else
                    _REMOVE_ATTRIBUTE(*attr, AM_HID);
                break;
            case 'S':
                if (opt == '+')
                    _ADD_ATTRIBUTE(attribute, AM_SYS);
                else
                    _REMOVE_ATTRIBUTE(*attr, AM_SYS);
                break;
            case 'A':
                if (opt == '+')
                    _ADD_ATTRIBUTE(attribute, AM_ARC);
                else
                    _REMOVE_ATTRIBUTE(*attr, AM_ARC);
                break;
            default:
                fprintf(stderr, "无效的属性: %c\n", attributes[i]);
                return -1;
        }
    }
#undef _ADD_ATTRIBUTE
#undef _REMOVE_ATTRIBUTE
    *attr |= attribute;
    return 0;
}

int shell_do_chmod(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "用法: chmod +/-<attr> [...] <path>\n");
        return -1;
    }

    BYTE         attr = 0, mask = 0;
    const TCHAR *filename = NULL;
    for (int i = 0; i < argc; ++i) {
        if (argv[i][0] == '+' || argv[i][0] == '-') {
            size_t attributes_len = strlen(argv[i]);
            if (attributes_len == 1) {
                fprintf(stderr, "用法: chmod +/-<attr> [...] <path>\n");
                return -1;
            }
            _deal_with_attributes(&attr, argv[i] + 1, attributes_len - 1, argv[i][0]);
            _deal_with_attributes(&mask, argv[i] + 1, attributes_len - 1, '+');
        } else {
            if (filename) {
                fprintf(stderr, "用法: chmod +/-<attr> [...] <path>\n");
                return -1;
            }
            filename = argv[i];
        }
    }

    FRESULT fr = f_chmod(filename, attr, mask);
    if (fr != FR_OK) {
        fprintf(stderr, "更改属性失败: %s (%s: %d)\n", filename, f_strerror(fr), fr);
        return -1;
    }
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

    if (argc > 1) {
        fprintf(stderr, "用法: getfree [<drive>]\n");
        return -1;
    }

    // 如果没有参数，则显示所有可能驱动器的信息
    if (argc == 0) {
        printf("驱动器列表 (最大支持 %d 个驱动器):\n", FF_VOLUMES);

        int mounted_count = 0;
        // 遍历所有可能的驱动器（根据FF_VOLUMES配置）
        for (int i = 0; i < FF_VOLUMES; i++) {
            // 构造驱动器路径
            TCHAR driver_path[4] = {0};
            if (FF_STR_VOLUME_ID == 0) {
                driver_path[0] = '0' + i;  // 数字驱动器号
                driver_path[1] = ':';
            }

            // 尝试获取驱动器的空闲空间信息
            FRESULT fr = f_getfree(driver_path, &fre_clust, &fs);
            if (fr == FR_OK) {
                mounted_count++;
                tot_sect = (fs->n_fatent - 2) * fs->csize;  // 总扇区数
                fre_sect = fre_clust * fs->csize;           // 空闲扇区数

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
            } else {
                printf("  驱动器 %d: 获取信息失败 (%s: %d)\n", i, f_strerror(fr), fr);
            }
        }

        if (mounted_count == 0) {
            printf("  没有已挂载的驱动器\n");
        }

        return 0;
    }

    // 如果提供了驱动器参数，则显示指定驱动器的信息
    FRESULT fr = f_getfree(argv[0], &fre_clust, &fs);
    if (fr != FR_OK) {
        if (fr == FR_INVALID_DRIVE) {
            printf("驱动器 %s 不存在\n", argv[0]);
        } else {
            fprintf(stderr, "获取空闲空间失败: %s (%s: %d)\n", argv[0], f_strerror(fr), fr);
        }
        return -1;
    }

    tot_sect = (fs->n_fatent - 2) * fs->csize;  // 总扇区数
    fre_sect = fre_clust * fs->csize;           // 空闲扇区数

    printf("驱动器 %s 的空闲空间:\n", argv[0]);
    printf("  总扇区数: %lu\n", (unsigned long)tot_sect);
    printf("  空闲扇区数: %lu\n", (unsigned long)fre_sect);
    printf("  簇大小: %u 扇区\n", (unsigned int)fs->csize);

    return 0;
}

int shell_do_getlabel(int argc, char **argv)
{
    if (argc != 1) {
        fprintf(stderr, "用法: getlabel <驱动器>\n");
        return -1;
    }

    TCHAR label[12];
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
    if (argc != 2) {
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
    if (argc != 2) {
        fprintf(stderr, "用法: export <源路径> <目标路径>\n");
        return -1;
    }

    const TCHAR *src_path = argv[0];  // 文件系统中的源路径
    const TCHAR *dst_path = argv[1];  // 宿主机文件系统的目标路径

    // 检查源路径是否存在
    FILINFO fno;
    FRESULT fr = f_stat(src_path, &fno);
    if (fr != FR_OK) {
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
        DIR dp;
        fr = f_opendir(&dp, src_path);
        if (fr != FR_OK) {
            fprintf(stderr, "无法打开目录: %s\n", src_path);
            return -1;
        }

        // 遍历目录中的所有文件和子目录
        while (1) {
            fr = f_readdir(&dp, &fno);
            if (fr != FR_OK || fno.fname[0] == 0)
                break;

            // 跳过当前目录和父目录项
            if (strcmp(fno.fname, ".") == 0 || strcmp(fno.fname, "..") == 0)
                continue;

            // 构造源路径和目标路径
            TCHAR full_src_path[256];
            TCHAR full_dst_path[256];

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

        f_closedir(&dp);
    } else {
        // 如果是文件，复制文件内容
        FIL   src_file;
        FILE *dst_file;

        // 打开源文件（FAT文件系统中的文件）
        fr = f_open(&src_file, src_path, FA_READ);
        if (fr != FR_OK) {
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
        TCHAR  buff[1024];  // 每次拷贝1024字节
        UINT   bytes_read;
        size_t bytes_written;

        while (1) {
            fr = f_read(&src_file, buff, sizeof(buff), &bytes_read);
            if (fr != FR_OK || bytes_read == 0)
                break;

            bytes_written = fwrite(buff, 1, bytes_read, dst_file);
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
        } else if (_shell_cmd0_is(head)) {
            ret = shell_do_head(argc - 1, argv + 1);
        } else if (_shell_cmd0_is(truncate)) {
            ret = shell_do_truncate(argc - 1, argv + 1);
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
