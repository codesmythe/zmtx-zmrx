
#include <stdio.h>
#include <sys/stat.h>
#include <utime.h>

#include "fileio.h"

long fileio_get_modification_time(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
        return -1;
    struct stat s;
    fstat(fileno(fp), &s);
    return s.st_mtime;
}

void fileio_set_modification_time(const char *filename, long mdate) {
    /*
     * set the time
     */
    struct utimbuf tv;
    tv.actime = (time_t)mdate;
    tv.modtime = (time_t)mdate;

    utime(filename, &tv);
}

long get_file_size(FILE *fp) {
    struct stat s;
    fstat(fileno(fp), &s);
    return s.st_size;
}
