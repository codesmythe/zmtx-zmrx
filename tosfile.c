
#include <stdio.h>
#include <sys/stat.h>

#include "fileio.h"

int use_aux = 0;

long fileio_get_modification_time(const char *filename) {
    return 0;
#if 0
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) return -1;
    struct stat s;
    fstat(fileno(fp), &s);
    return s.st_mtime;
#endif
}

void fileio_set_modification_time(const char *filename, long mdate) {
}

long get_file_size(FILE *fp) {
     return 0;
}

char *strip_path(char *path_in)
{
    // Nothing in particular to do for ST or unix.
    return path_in;
}

int validate_device_choice(char choice)
{
    // All choices are valid at this point.
    return 1;
}

int get_matching_files(uint8_t *result, int argc, char **argv)
{
    return 0;
}
