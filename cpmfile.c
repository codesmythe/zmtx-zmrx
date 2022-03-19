#include <cpm.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "fileio.h"

long fileio_get_modification_time(const char *filename) { return 0L; }

void fileio_set_modification_time(const char *filename, long mdate) {}

/*

long get_file_size(FILE *fp) {

    long size;

    int F_SIZE = 35;  // BDOS function call for file size in sectors
    
    int sector_size = 128;  // sectors in CP/M are 128 bytes each
    
    // compute maximum file size (number of sectors in file) * sector size 
    size = bdos(F_SIZE, *getfcb(void)) * sector_size;
    
    return size;
}

*/

// code sample from geeksforgeeks modified for zmtx

long get_file_size(FILE *fp)
{
    // assumes file fp is already open

    // checking if the file exist or not
    if (fp == NULL) {
        printf("File Not Found!\n");
        return -1;
    }

    fseek(fp, 0L, SEEK_END);

    // calculating the size of the file
    long res = ftell(fp);

    // leave file open

    return res;
}

/// Remove path components a path.
/**
 * For CP/M, we remove the drive letter, if any.
 * @param path_in Input pathname
 * @return Pathname, with drive letter and color removed.
 */

char *strip_path(char *path_in)
{
    char *p = strrchr(path_in, ':');
    if (p == NULL) {
        p = path_in;
    } else {
        p++;
    }
    return p;
}

int validate_device_choice(char choice) {
    if (choice== '0') return 1;
    else if (choice == '1') {
        /*
         * We don't allow use of aux device on CP/M 2.2 because I don't
         * see a way to check the status of AUXOUT on that OS.
         */
        int version_number = bdos(CPM_VERS, 0);
        if (version_number < 0x30) {
            printf("zmtx: AUX device not supported for CP/M version < 3.0\n");
            printf("      because there is no function to get AUX device status.\n");
            return 0;
        }
        return 1;
    } else {
        printf("zmtx: bad value for -X, expected '0' or '1', got '%s'\n", optarg);
        return 0;
    }
    return 1;
}


/**** Code for wildcard handling ****/

/// Extract a file name from a CP/M directory entry.
/**
 * Look into directory buffer 'dirbuf' at position 'dirpos'
 * and extract the file name (base plus extension) from the
 * directory entry. CP/M stores attribute bits in the filename,
 * so those are stripped. Result buffer should be at least 15 bytes.
 *
 * @param result The full filename, include drive letter.
 * @param dirbuf A CP/M directory buffer.
 * @param dirpos An index (0..3) of the directory entry in the buffer.
 * @param The CP/M drive number (A: = 1, B: = 2, etc.).
 */
static void get_dir_entry_name(char *result, char *dirbuf, int dirpos, char drive_num)
{
    char *source = dirbuf + 32 * dirpos; // base of the 32-byte directory entry

    *result++ = '@' + drive_num;
    *result++ = ':';

    /* Handle the base name part. */
    for (int i = 0; i < 8; i++) {
        char ch = *(source + i + 1);
        if (ch == ' ') break;
        *result++ = ch & 0x7f; // Strip possible attribute bit.
    }
    *result++ = '.';

    /* Handle the extension part. */
    for (int i = 0; i < 3; i++) {
        char ch = *(source + i + 9);
        if (ch == ' ') break;
        *result++ = ch & 0x7f; // Strip possible attribute bit.
    }
    *result = 0;
}

/// Initialize an entry File Control Block (FCB) from a filename pattern.
/**
 *
 * @param dirbuf Pointer to a CP/M directory buffer.
 * @param dirpos An index (0..3) of the directory entry in the buffer.
 * @param pattern Filename or wildcard pattern like 'WS*.*'
 * @param quiet If true, don't print error messages.
 * @return A pointer to an initialized File Control Block (FCB).
 */
static struct fcb *init_fcb(char *dirbuf, char *dirpos, char *pattern, int quiet) {
    // Make a library call to get a new FCB.
    struct fcb *the_fcb = (struct fcb *) getfcb();
    // Return an error if we get a NULL FCB.
    if (the_fcb == NULL) {
        if (!quiet) fprintf(stderr, "Error: unable to get a free File Control Block (FCB). Can't continue.\n");
        return NULL;
    }
    // Tell CP/M where the directory buffer should be.
    bdos(CPM_SDMA, (int)dirbuf);
    // Zero out the FCB.
    memset(the_fcb, 0, sizeof(struct fcb));
    // Set the base and extention parts of the FCB based on the incoming pattern.
    setfcb(the_fcb, (unsigned char *) pattern);
    // Ask CP/M for the first entry.
    *dirpos = bdos(CPM_FFST, (int) the_fcb);
    // If we get -1, there are no matches for the pattern.
    if (*dirpos == -1) {
        if (!quiet) fprintf(stderr, "Note: pattern \"%s\" did not match any files.\n", pattern);
    }
    return the_fcb;
}

/// Populate array of filenames that match a given file or pattern, up to max_matches.
/**
 *
 * @param result Buffer to contain matched filenames.
 * @param result_size The size of the matched filenames buffer in bytes.
 * @param pattern The single filename or wildcard pattern to search.
 * @return NULL if too many matches to fit in result buffer, otherwise pointer to next free byte in result.
 */

#define FILENAME_SIZE 16

uint8_t* get_matching_files_for_pattern(uint8_t *result, uint16_t *result_size, int *match_count, char *pattern)
{
    char dirbuf[134];
    char filename[FILENAME_SIZE];
    char dirpos = -1;
    struct fcb *the_fcb = init_fcb(dirbuf, &dirpos, pattern, 0);
    char drive_num = the_fcb->drive;
    int buffer_remaining = *result_size;

    if (drive_num == 0) drive_num = bdos(CPM_IDRV, 0) + 1;

    while (dirpos != -1) {
        get_dir_entry_name(filename, dirbuf, dirpos, drive_num);
        uint8_t len = strlen(filename);
        if (buffer_remaining - len - 2 < 0) {
            fprintf(stderr, "Error: Too many filenames (not enough room in filename buffer).\r\n");
            return NULL;
        }
        *result++ = len;
        strncpy((char *) result, filename, len);
        result += len;
        buffer_remaining -= len + 1;
        *match_count = *match_count + 1;
        dirpos = bdos(CPM_FNXT, (int) the_fcb);
    }
    *result_size  = buffer_remaining;
    return result;
}

/// Populate array of filenames that match files or wildcard patterns in argv list.
/**
 *
 * @param result Buffer to contain matched filenames.
 * @param result_size The size of the matched filenames buffer in bytes.
 * @param argc Number of items in argv pattern list.
 * @param argv List of patterns or full files names to search.
 * @return Result buffer filled with found filenames. For each file name entry, the first byte
 *         is the length of the filename, follow by that many bytes (not zero terminated). The
 *         value 0xFF for the length marks the end of the list.
 *
 *         The total number of matches is returned.
 *
 * Filenames have driver letter, colon, base name, dot, extention. For example, B:RLX16.COM.
 */

int get_matching_files(uint8_t *result, uint16_t result_size, int argc, char **argv)
{
    int total_matches = 0;
    for (int i = 0; i < argc; i++) {
        result = get_matching_files_for_pattern(result, &result_size, &total_matches, argv[i]);
        if (result == NULL) return -1;
    }
    *result = 0xff;
    return total_matches;

}
