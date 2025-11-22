#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmd.h"
#include "fferrno.h"

typedef struct mount_cmd_args_t {
    char* img_path;
    char* driver_number;
} mount_cmd_args_t;

const char* mount_help_str =
    "用法: mount [选项]\n"
    "挂载虚拟磁盘镜像。\n\n"
    "选项:\n"
    "  -p, --img-path=/path/to/img  指定虚拟磁盘镜像的名称。(默认: disk.img)\n"
    "  -d, --driver-number=数值     指定要使用的驱动器编号。(默认: 0)\n"
    "  -h, --help                   显示此帮助信息。\n";

static const mount_cmd_args_t default_args = {
    .img_path      = "disk.img",
    .driver_number = "",
};

cmd_args_t cmd_parse_mount_args(int argc, char** argv)
{
    mount_cmd_args_t* args = (mount_cmd_args_t*)calloc(1, sizeof(mount_cmd_args_t));
    if (!args) {
        return NULL;
    }

    *args = default_args;

    static struct option long_options[] = {{"img-path", optional_argument, NULL, 'p'},
                                           {"driver-number", optional_argument, NULL, 'd'},
                                           {"help", no_argument, NULL, 'h'},
                                           {0, 0, 0, 0}};

    int opt;
    int opt_index = 0;
    while ((opt = getopt_long(argc, argv, "p:d:h", long_options, &opt_index)) != -1) {
        switch (opt) {
            case 'p':
                args->img_path = strdup(optarg);
                break;
            case 'd':
                args->driver_number = strdup(optarg);
                break;
            case 'h':
                printf("%s", mount_help_str);
                cmd_free_mount_args(args);
                return MONO_ARGS_VALUE;
            default:
                fprintf(stderr, "未知选项: %c\n", opt);
                cmd_free_mount_args(args);
                return NULL;
        }
    }

    if (!args->img_path) {
        fprintf(stderr, "必需参数: --img-name=名称\n");
        cmd_free_mount_args(args);
        return NULL;
    }

    return (cmd_args_t)args;
}

void cmd_free_mount_args(cmd_args_t arg)
{
    mount_cmd_args_t* args = cmd_args_cast(arg, mount_cmd_args_t);
    if (args) {
        cmd_args_field_should_free(args, img_path, default_args);
        cmd_args_field_should_free(args, driver_number, default_args);
        free(args);
    }
}

int cmd_do_mount(cmd_args_t arg)
{
    mount_cmd_args_t* args = cmd_args_cast(arg, mount_cmd_args_t);
    if (!args) {
        return -1;
    }

    extern char* disk_path;
    disk_path = args->img_path;

    FATFS   fs;
    FRESULT fr = f_mount(&fs, args->driver_number, 0);
    if (fr != FR_OK) {
        fprintf(stderr, "挂载虚拟磁盘镜像失败: %s (%s: %d)\n", args->img_path, f_strerror(fr), fr);
        return -1;
    }

    // printf("Mounted virtual disk image %s to driver %s.\n", args->img_path, args->driver_number);
    shell_run();

    fr = f_unmount(args->driver_number);
    if (fr != FR_OK) {
        fprintf(stderr, "卸载虚拟磁盘镜像失败: %s (%s: %d)\n", args->img_path, f_strerror(fr), fr);
        return -1;
    }

    return 0;
}
