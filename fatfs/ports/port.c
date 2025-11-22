#include <time.h>
#include "ff.h"

/**
 * 获取读写时间
 */
DWORD get_fattime(void) {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    return ((DWORD)(timeinfo->tm_year - 80) << 25) |
           ((DWORD)(timeinfo->tm_mon + 1) << 21) |
           ((DWORD)timeinfo->tm_mday << 16) |
           ((DWORD)timeinfo->tm_hour << 11) |
           ((DWORD)timeinfo->tm_min << 5) |
           ((DWORD)timeinfo->tm_sec >> 1);
}