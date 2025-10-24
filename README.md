# FAT文件系统工具 (fatfs-tool)

一个基于fatfs构建的简单FAT文件系统工具，用于创建和管理虚拟FAT磁盘镜像。

## 功能特性

- 创建FAT格式的虚拟磁盘镜像
- 格式化现有的虚拟磁盘镜像
- 挂载虚拟磁盘镜像并进行文件操作
- 支持常见的文件系统操作命令：
  - `ls` - 列出目录内容
  - `cd` - 切换目录
  - `pwd` - 显示当前目录
  - `mkdir` - 创建目录
  - `rm` - 删除文件
  - `read` - 读取文件内容
  - `write` - 写入文件内容
  - `stat` - 检查文件/目录状态
  - `mv` - 移动/重命名文件或目录
  - `chmod` - 更改文件/目录属性
  - `getfree` - 获取卷空闲空间
  - `getlabel` - 获取卷标
  - `setlabel` - 设置卷标
  - `export` - 导出文件/目录到宿主机文件系统
  - `clear` - 清空屏幕
  - `help` - 显示帮助信息
  - `exit` - 退出shell

## 依赖

- getopt库

## 使用方法

### 命令行模式

```bash
# 显示帮助信息
./fat-tool help

# 显示版本信息
./fat-tool version

# 创建虚拟磁盘镜像
./fat-tool create <image-file> [size-in-MB]

# 格式化虚拟磁盘镜像
./fat-tool format <image-file> [format]

# 挂载虚拟磁盘镜像并进入交互模式
./fat-tool mount <image-file>
```

### 交互模式

运行 `./fat-tool mount <image-file>` 后会进入交互式shell，可以执行各种文件系统操作命令：

```bash
# 写入文件（支持带引号的空格字符串）
write test.txt "Looking into your past is the only way to discover your future."

# 读取文件内容
read test.txt

# 列出目录内容
ls /

# 创建目录
mkdir /mydir

# 导出文件或目录到宿主机
export /test.txt ./exported_test.txt
export /mydir ./exported_mydir

# 其他命令...
```

## 项目结构

```
├── fatfs/              # fatfs移植到本地文件系统的源码
├── src/                # 工具源码
│   ├── cmd/            # 各种命令实现
│   │   ├── builtin.c   # 内置命令(help, version)
│   │   ├── create.c    # 创建磁盘命令
│   │   ├── format.c    # 格式化磁盘命令
│   │   ├── mount.c     # 挂载磁盘命令
│   │   └── shell.c     # 交互式shell命令
│   └── main.c          # 主程序入口
├── CMakeLists.txt
└── README.md
```
