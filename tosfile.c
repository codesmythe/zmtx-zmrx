
#include <stdio.h>
#include <stdint.h>
#include <string.h>
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

static uint8_t *find_files(const char *path, uint8_t *result, uint16_t *result_size) {
    char dirbuf[PATH_MAX];

    /* dirname(3) can writeto its arg, so use buffer */
    strcpy(dirbuf, path);
    const char *dirp = dirname(dirbuf);
    uint8_t dirlen = strlen(dirp);

    _DTA local_dta, *saved_dta = Fgetdta();

    Fsetdta(&local_dta);
    uint16_t remaining = *result_size;
    uint32_t err = Fsfirst (path, FA_RDONLY | FA_CHANGED);
    while (err == 0) {
        uint8_t namelen = strlen(local_dta.dta_name);
        uint8_t pathlen = namelen + dirlen + 1;
        if (remaining - pathlen - 2 < 0) {
            fprintf(stderr, "Error: Too many filenames (not enough room in filename buffer).\r\n");
            return NULL;
        }
        *result++ = pathlen;
        strcpy((char *) result, dirp);
        strcat((char *) result, "\\");
        strncat((char *) result, local_dta.dta_name, namelen);
        result += pathlen;
        remaining -= pathlen + 1;

        err = Fsnext ();
    }
    Fsetdta(saved_dta);
    *result_size = remaining;
    return result;
}

int get_matching_files(uint8_t *result, uint16_t result_size, int argc, char **argv)
{
    int count = 0;
    uint8_t *p = result;
    for (int i = 0; i < argc; i++) {
        p = find_files(argv[i], p, &result_size);
        if (p == NULL) return -1;
    }
    *p = 0xFF;
    /* Count how many files we found */
    p = result;
    while (*p != 0xff) {
        count++;
        p += (*p) + 1;
    }
    return count;
}
