
#include <stdio.h>
#include <sys/stat.h>
#include <utime.h>

#include "fileio.h"

int use_aux = 0;

long fileio_get_modification_time(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
        return 0;
    struct stat s;
    fstat(fileno(fp), &s);
    return s.st_mtime;
}

void fileio_set_modification_time(const char *filename, long mdate)
{
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

char *strip_path(char *path_in)
{
    // Nothing in particular to do for unix.
    return path_in;
}

int validate_device_choice(char choice)
{
    // All choices are valid at this point.
    return 1;
}

int get_matching_files(char *result, int argc, char **argv)
{
    return 0;
}

