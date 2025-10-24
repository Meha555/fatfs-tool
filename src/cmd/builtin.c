#include "cmd.h"
#include <stdio.h>

#define VERSION "R.016"

int cmd_do_help(cmd_args_t arg)
{
    (void)arg;
    printf("用法: "PROGRAM_NAME" <命令> [参数...]\n");
    printf("一个基于fatfs构建的简单FAT文件系统工具。\n");
    printf("\n命令:\n");
    printf("  help                    显示此帮助信息。\n");
    printf("  version                 显示版本信息。\n");
    printf("  create                  创建虚拟磁盘镜像。\n");
    printf("  format                  格式化虚拟磁盘镜像。\n");
    printf("  mount                   挂载虚拟磁盘镜像。\n");
    return 0;
}

int cmd_do_version(cmd_args_t arg)
{
    (void)arg;
    printf("%s\n", VERSION);
    return 0;
}
