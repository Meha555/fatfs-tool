#include <stdio.h>
#include "ff.h"
#include "diskio.h"
#include "config.h"

// 全局文件指针，指向虚拟磁盘文件
extern char *disk_path;
static FILE* vdisk_fp = NULL;
static DWORD total_sectors = 0; // 总扇区数

// 打开虚拟磁盘
DSTATUS disk_initialize(BYTE pdrv) {
    (void)pdrv;  // 忽略驱动器号（仅一个虚拟磁盘）
    if (disk_path) {
        vdisk_fp = fopen(disk_path, "rb+");  // 读写模式打开
        if (vdisk_fp) {
            // 获取文件大小
            if (fseek(vdisk_fp, 0, SEEK_END) == 0) {
                long file_size = ftell(vdisk_fp);
                if (file_size >= 0) {
                    // 计算总扇区数
                    total_sectors = (DWORD)(file_size / SECTOR_SIZE);
                }
                // 将文件指针重置到文件开始
                fseek(vdisk_fp, 0, SEEK_SET);
            } else {
                return STA_NOINIT;
            }
        }
    }
    return (vdisk_fp != NULL) ? RES_OK : STA_NOINIT;
}

// 获取磁盘状态
DSTATUS disk_status(BYTE pdrv) {
    (void)pdrv;
    return (vdisk_fp != NULL) ? RES_OK : STA_NOINIT;
}

// 读取扇区
DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
    (void)pdrv;
    if (!vdisk_fp) return RES_NOTRDY;

    // 定位到扇区位置
    if (fseek(vdisk_fp, sector * SECTOR_SIZE, SEEK_SET) != 0) {
        return RES_ERROR;
    }

    // 读取count个扇区
    if (fread(buff, SECTOR_SIZE, count, vdisk_fp) != count) {
        return RES_ERROR;
    }

    return RES_OK;
}

// 写入扇区
DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
    (void)pdrv;
    if (!vdisk_fp) return RES_NOTRDY;

    // 定位到扇区位置
    if (fseek(vdisk_fp, sector * SECTOR_SIZE, SEEK_SET) != 0) {
        return RES_ERROR;
    }

    // 写入count个扇区
    if (fwrite(buff, SECTOR_SIZE, count, vdisk_fp) != count) {
        return RES_ERROR;
    }

    return RES_OK;
}

// 控制操作
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
    (void)pdrv;  // 忽略驱动器号（本项目只有一个虚拟磁盘）
    switch (cmd) {
        case CTRL_SYNC:
            // 功能：完成待处理的写操作
            fflush(vdisk_fp);
            return RES_OK;

        case GET_SECTOR_COUNT:
            // 功能：获取总扇区数（格式化必需）
            *(DWORD*)buff = total_sectors;
            return RES_OK;

        case GET_SECTOR_SIZE:
            // 功能：获取扇区大小
            *(WORD*)buff = SECTOR_SIZE;
            return RES_OK;

        case GET_BLOCK_SIZE:
            // 功能：获取擦除块大小（虚拟磁盘设为1扇区，格式化时用于计算簇大小）
            *(DWORD*)buff = 1;
            return RES_OK;

        case CTRL_TRIM:
            // 功能：通知设备指定扇区数据不再使用（虚拟文件无需实际擦除，返回成功）
            // 注：仅当FF_USE_TRIM == 1时FatFs才会调用
            return RES_OK;

        case CTRL_POWER:
        case CTRL_LOCK:
        case CTRL_EJECT:
        case CTRL_FORMAT:
        case MMC_GET_TYPE:
        case MMC_GET_CSD:
        case MMC_GET_CID:
        case MMC_GET_OCR:
        case MMC_GET_SDSTAT:
        case ISDIO_READ:
        case ISDIO_WRITE:
        case ISDIO_MRITE:
        case ATA_GET_REV:
        case ATA_GET_MODEL:
        case ATA_GET_SN:
        default:
            return RES_PARERR;  // 未支持的命令
    }
}