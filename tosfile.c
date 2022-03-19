
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ext.h>

#include "fileio.h"

int use_aux = 0;

static void parse_ftime(struct ftime *result, uint32_t dtval)
{
    result->ft_hour  = (dtval >> 27) & 0x1f;
    result->ft_min   = (dtval >> 21) & 0x3f;
    result->ft_tsec  = (dtval >> 16) & 0x1f;
    result->ft_year  = (dtval >>  9) & 0x3f;
    result->ft_month = (dtval >>  5) & 0xf;
    result->ft_day   =  dtval        & 0x1f;
}

static struct ftime tmtoftime(struct tm *tm)
{
    struct ftime tostime;
    tostime.ft_day = tm->tm_mday;
    tostime.ft_month = tm->tm_mon + 1;
    tostime.ft_year = tm->tm_year - 80;
    tostime.ft_hour = tm->tm_hour;
    tostime.ft_min  = tm->tm_min;
    tostime.ft_tsec = tm->tm_sec / 2;
    return tostime;
}

time_t fileio_get_modification_time(const char *filename) {
    struct stat s;
    struct ftime tosftime;

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) return 0;

    int rc = fstat(fileno(fp), &s);
    fclose(fp);
    if (rc < 0) return 0;
    /*
     * The time-related items in stat are stored as TOS DOSTIME values.
     * But main program deals with UNIX time (seconds since the epoch),
     * so need to convert.
     */
    parse_ftime(&tosftime, s.st_mtime);
    struct tm* my_tm = ftimtotm(&tosftime);
    return mktime(my_tm);
}

void fileio_set_modification_time(const char *filename, time_t mdate) {
    struct tm *tm = localtime(&mdate);
    struct ftime tostime = tmtoftime(tm);

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) return;

    Fdatime(&tostime, fileno(fp), 1);
    fclose(fp);
}

long get_file_size(FILE *fp) {
    if (fp == NULL) return 0;
    struct stat s;
    fstat(fileno(fp), &s);
    return (long) s.st_size;
}

char *strip_path(char *path_in)
{
    return basename(path_in);
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
