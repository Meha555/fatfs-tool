#include "fferrno.h"

static const char *g_fferrors[] = {
    "Succeeded",
    "A hard error occurred in the low level disk I/O layer",
    "Internal error",
    "The physical drive does not work",
    "Could not find the file",
    "Could not find the path",
    "The path name format is invalid",
    "Access denied due to a prohibited access or directory full",
    "Access denied due to a prohibited access",
    "The file/directory object is invalid",
    "The physical drive is write protected",
    "The logical drive number is invalid",
    "The volume has no work area",
    "Could not find a valid FAT volume",
    "The f_mkfs function aborted due to some problem",
    "Could not take control of the volume within defined period",
    "The operation is rejected according to the file sharing policy",
    "LFN working buffer could not be allocated, given buffer size is insufficient or too deep path",
    "Number of open files > FF_FS_LOCK",
    "Given parameter is invalid"
};

const char *f_strerror(FRESULT res)
{
    return g_fferrors[res];
}