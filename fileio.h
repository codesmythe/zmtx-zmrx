#ifndef __FILEIO_H__
#define __FILEIO_H__

#include <stdio.h>

#define MAX_MATCHES 512   /* Max number of filenames eturned by wildcard matcher. */
#define FILENAME_SIZE 16  /* Size of one filename entry */

long fileio_get_modification_time(const char *filename);
void fileio_set_modification_time(const char *filename, long mdate);
char *strip_path(char *path_in);

long get_file_size(FILE *fp);

int get_matching_files(char *result, int argc, char **argv);

#endif
