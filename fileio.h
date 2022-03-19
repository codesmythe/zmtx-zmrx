#ifndef __FILEIO_H__
#define __FILEIO_H__

#include <stdio.h>
#include <stdint.h>

#define FILENAME_BUFFER_SIZE 8192

time_t fileio_get_modification_time(const char *filename);
void fileio_set_modification_time(const char *filename, time_t mdate);
char *strip_path(char *path_in);

int validate_device_choice(char choice);

long get_file_size(FILE *fp);

int get_matching_files(uint8_t *result, uint16_t result_size, int argc, char **argv);

#endif
