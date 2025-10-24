#include <stdio.h>
#include <string.h>
#include "cmd/cmd.h"

char *disk_path = NULL;

// 命令映射表
static cmd_t cmd_map[] = {{"help", cmd_do_help, NULL, NULL},
                          {"version", cmd_do_version, NULL, NULL},
                          {"create", cmd_do_create, cmd_parse_reate_args, cmd_free_create_args},
                          {"format", cmd_do_format, cmd_parse_format_args, cmd_free_format_args},
                          {"mount", cmd_do_mount, cmd_parse_mount_args, cmd_free_mount_args},
                          {NULL, NULL, NULL, NULL}};

int main(int argc, char **argv)
{
    if (argc < 2) {
        cmd_do_help(NULL);
        return 0;
    }

    // 查找命令
    const char *cmd_name  = argv[1];
    int         cmd_found = 0;
    int         result    = 0;

    for (int i = 0; cmd_map[i].name != NULL; i++) {
        if (strcmp(cmd_name, cmd_map[i].name) == 0) {
            cmd_found = 1;
            // 解析参数（如果有解析函数）
            cmd_args_t args = NULL;
            if (cmd_map[i].parse_args) {
                args = cmd_map[i].parse_args(argc + 1, argv - 1);
                if (args == MONO_ARGS_VALUE) {
                    return 0;
                } else if (!args) {
                    return 1;
                }
            }
            // 执行命令
            result = cmd_map[i].exec(args);
            // 释放参数
            if (args && cmd_map[i].free_args) {
                cmd_map[i].free_args(args);
            }
            break;
        }
    }

    if (!cmd_found) {
        fprintf(stderr, "未知命令: %s\n", cmd_name);
        cmd_do_help(NULL);
        return 1;
    }

    return result;
}