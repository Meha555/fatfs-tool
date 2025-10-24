#ifndef TOOL_SRC_CMD_H_
#define TOOL_SRC_CMD_H_

#ifdef _WIN32
#include <getopt.h>
#else
#include <unistd.h> /* for getopt */
#endif
#include "config.h"
#include "ff.h"

typedef void *cmd_args_t;
typedef int (*cmd_func_t)(cmd_args_t);

#define MONO_ARGS_VALUE (cmd_args_t) - 1

typedef struct cmd_t {
    const char *name;
    cmd_func_t  exec;
    cmd_args_t (*parse_args)(int argc, char **argv);
    void (*free_args)(cmd_args_t args);
} cmd_t;

#define cmd_args_cast(args, type) ((type *)args)

#define cmd_args_field_should_free(args, field, default_args)                                      \
    do {                                                                                           \
        if ((args) && (args)->field && (args)->field != (default_args).field) {                    \
            free((args)->field);                                                                   \
        }                                                                                          \
    } while (0)

typedef struct create_cmd_args_t {
    char     *img_name;
    size_t    img_size;
    MKFS_PARM mkfs_parm;
} create_cmd_args_t;

typedef struct format_cmd_args_t {
    char     *img_path;
    MKFS_PARM mkfs_parm;
} format_cmd_args_t;

typedef struct mount_cmd_args_t {
    char *img_path;
    char *driver_number;
} mount_cmd_args_t;

// 命令执行函数声明
int cmd_do_help(cmd_args_t);
int cmd_do_version(cmd_args_t);
int cmd_do_create(cmd_args_t arg);
int cmd_do_format(cmd_args_t arg);
int cmd_do_mount(cmd_args_t arg);

// 命令解析函数声明
cmd_args_t cmd_parse_reate_args(int argc, char **argv);
void       cmd_free_create_args(cmd_args_t arg);
cmd_args_t cmd_parse_format_args(int argc, char **argv);
void       cmd_free_format_args(cmd_args_t arg);
cmd_args_t cmd_parse_mount_args(int argc, char **argv);
void       cmd_free_mount_args(cmd_args_t arg);

// Shell命令函数声明
int shell_do_help(int argc, char **argv);
int shell_do_ls(int argc, char **argv);
int shell_do_mkdir(int argc, char **argv);
int shell_do_rm(int argc, char **argv);
int shell_do_cd(int argc, char **argv);
int shell_do_pwd(int argc, char **argv);

int shell_do_read(int argc, char **argv);
int shell_do_write(int argc, char **argv);

int shell_do_touch(int argc, char **argv);
int shell_do_mv(int argc, char **argv);

int shell_do_stat(int argc, char **argv);
int shell_do_chmod(int argc, char **argv);

int shell_do_getfree(int argc, char **argv);
int shell_do_getlabel(int argc, char **argv);
int shell_do_setlabel(int argc, char **argv);

// 导出文件系统中的文件或目录到宿主机文件系统
int shell_do_export(int argc, char **argv);

int shell_run(void);

#endif  // TOOL_SRC_CMD_H_
