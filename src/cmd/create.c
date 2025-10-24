#include "cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORK_BUFFER_SIZE 4096 // 工作缓冲区大小
static char work_buffer[WORK_BUFFER_SIZE]; // 定义工作缓冲区

const char* create_help_str = "用法: create [选项]\n"
                              "创建虚拟磁盘镜像。\n\n"
                              "选项:\n"
                              "  -n, --name=名称    指定虚拟磁盘镜像的名称。(默认: disk.img)\n"
                              "  -s, --size=大小    指定虚拟磁盘镜像的大小(MB)。(必填)\n"
                              "  -f, --fmt=类型     指定FAT类型 (FAT12, FAT16, FAT32, EXFAT, SFD)。\n"
                              "  --n-fat=数量       指定FAT表的数量 (0=默认)。\n"
                              "  --align=数值       指定数据区域的扇区对齐大小 (0=默认)。\n"
                              "  --n-root=数量      指定根目录的数量 (0=默认)。\n"
                              "  --au-size=大小     指定簇大小(字节) (0=默认)。\n";

static const create_cmd_args_t default_args = {
    .img_name = "disk.img",
    .mkfs_parm = {
        // FAT文件系统格式：
        /*
        FM_FAT：创建 FAT12 或 FAT16（由存储介质大小自动决定，小容量为 FAT12，中容量为 FAT16）。
        FM_FAT32：强制创建 FAT32（适用于大容量存储，通常 >32MB）。
        FM_EXFAT：创建 exFAT（支持单个文件 >4GB，适用于大容量存储）。
        FM_SFD：创建 “小软盘格式”（Small Floppy Disk，针对极小容量设备，如早期软盘，通常 <16MB，会自动选择 FAT12）。
        */
        .fmt = FM_FAT,
        .n_fat = 0, // FAT 表数量：若设为 0，FatFs 会自动使用默认值（通常为 2 个副本，主 FAT + 备份 FAT，提高可靠性）
        .align = 0, // 数据区（Data Area）的起始对齐扇区数（单位：扇区，1 扇区通常为 512 字节）：若设为 0，FatFs 会自动选择最优对齐值（通常对齐到 1 扇区，即不额外偏移）
        .n_root = 0, // 根目录数量：若设为 0，FatFs 会根据文件系统类型自动分配默认值（推荐）
        .au_size = 0, // 簇字节数：若设为 0，FatFs 会根据存储介质总容量自动选择最优簇大小（推荐）
    },
};

// 解析create命令参数
cmd_args_t cmd_parse_reate_args(int argc, char** argv)
{
    create_cmd_args_t* args = (create_cmd_args_t*)calloc(1, sizeof(create_cmd_args_t));
    if (!args) {
        return NULL;
    }

    // 设置默认值
    *args = default_args; // 为啥这里显示default_args的img_name是NULL？？？？？

    static struct option long_options[] = {
        { "name", optional_argument, 0, 'n' },
        { "size", required_argument, 0, 's' }, // MB
        { "fmt", optional_argument, 0, 'f' },
        { "n-fat", optional_argument, 0, 4 },
        { "align", optional_argument, 0, 5 },
        { "n-root", optional_argument, 0, 6 },
        { "au-size", optional_argument, 0, 7 },
        { "help", no_argument, 0, 'h' },
        { 0, 0, 0, 0 }
    };

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "n:s:f:h", long_options, &option_index)) != -1) {
        switch (opt) {
        case 'n': // name
            cmd_args_field_should_free(args, img_name, default_args);
            args->img_name = strdup(optarg);
            break;
        case 's': // size
            args->img_size = atoi(optarg);
            break;
        case 'f': // fmt
            args->mkfs_parm.fmt = atoi(optarg);
            break;
        case 4: // n-fat
            args->mkfs_parm.n_fat = atoi(optarg);
            break;
        case 5: // align
            args->mkfs_parm.align = atoi(optarg);
            break;
        case 6: // n-root
            args->mkfs_parm.n_root = atoi(optarg);
            break;
        case 7: // au-size
            args->mkfs_parm.au_size = atoll(optarg);
            break;
        case 'h': // help
            printf("%s", create_help_str);
            cmd_free_create_args(args);
            return MONO_ARGS_VALUE;
        default:
            fprintf(stderr, "未知选项: %c\n", opt);
            cmd_free_create_args(args);
            return NULL;
        }
    }

    // 验证必需参数
    if (args->img_size == 0) {
        fprintf(stderr, "必需参数: --size=大小\n");
        cmd_free_create_args(args);
        return NULL;
    }

    return (cmd_args_t)args;
}

// 释放create命令参数
void cmd_free_create_args(cmd_args_t arg)
{
    create_cmd_args_t* args = cmd_args_cast(arg, create_cmd_args_t);
    if (args) {
        if (args->img_name) {
            cmd_args_field_should_free(args, img_name, default_args);
        }
        free(args);
    }
}

// 创建指定大小的虚拟磁盘文件
int create_virtual_disk(const char* path, int total_sectors)
{
    FILE* fp = fopen(path, "wb");
    if (!fp) {
        return -1;
    }

    // 填充空数据（总大小=扇区数×SECTOR_SIZE）
    char* buffer = (char*)calloc(1, SECTOR_SIZE);
    if (!buffer) {
        fclose(fp);
        return -1;
    }

    // 按照扇区写入，一次性写入可能会崩，受限于硬盘带宽
    for (int i = 0; i < total_sectors; i++) {
        if (fwrite(buffer, 1, SECTOR_SIZE, fp) != SECTOR_SIZE) {
            free(buffer);
            fclose(fp);
            return -1;
        }
    }

    free(buffer);
    fclose(fp);
    return 0;
}

// create命令执行函数
int cmd_do_create(cmd_args_t arg)
{
    create_cmd_args_t* args = (create_cmd_args_t*)arg;
    if (!args || !args->img_name) {
        fprintf(stderr, "无效的参数\n");
        return -1;
    }

    if (create_virtual_disk(args->img_name, args->img_size * MB / SECTOR_SIZE) != 0) {
        fprintf(stderr, "创建虚拟磁盘文件失败: %s\n", args->img_name);
        return -1;
    }

    extern char* disk_path;
    disk_path = args->img_name;

    // 创建文件系统
    FRESULT fr = f_mkfs("", &args->mkfs_parm, work_buffer, sizeof(work_buffer));
    if (fr != FR_OK) {
        remove(args->img_name);
        fprintf(stderr, "创建文件系统失败！错误代码: %d\n", fr);
        return -1;
    }

    printf("虚拟磁盘创建成功: %s (%zu MB)\n", args->img_name, args->img_size);
    return 0;
}