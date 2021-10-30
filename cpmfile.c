

#include "fileio.h"
#include <stdlib.h>

long fileio_get_modification_time(const char *filename) { return -1L; }

void fileio_set_modification_time(const char *filename, long mdate) {}

long get_file_size(FILE *fp) {
    fprintf(stderr, "Sorry, get_file_size() has not been implemented for CP/M.\n");
    exit(1);
    return -1;
}
