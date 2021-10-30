#ifndef __FILEIO_H__
#define __FILEIO_H__

#include <stdio.h>

long fileio_get_modification_time(const char *filename);
void fileio_set_modification_time(const char *filename, long mdate);
long get_file_size(FILE *fp);

#endif
