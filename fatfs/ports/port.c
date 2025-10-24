#include "ff.h"

/**
 * 获取读写时间
 */
DWORD get_fattime(void) {
#if defined(_WIN32) || defined(_WIN64)
    SYSTEMTIME st;
    GetLocalTime(&st);

    return ((DWORD)(st.wYear - 1980) << 25) |
           ((DWORD)st.wMonth << 21) |
           ((DWORD)st.wDay << 16) |
           ((DWORD)st.wHour << 11) |
           ((DWORD)st.wMinute << 5) |
           ((DWORD)st.wSecond >> 1);
#elif defined(__unix__) || defined(__unix) || defined(__linux__) || defined(__APPLE__) || defined(__MACH__)
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
#else
    // 默认实现（固定时间）
    // 2023-1-1 00:00:00
    return ((DWORD)(2023 - 1980) << 25)
        | ((DWORD)1 << 21)
        | ((DWORD)1 << 16)
        | ((DWORD)0 << 11)
        | ((DWORD)0 << 5)
        | ((DWORD)0 >> 1);
#endif
}