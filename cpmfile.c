

#include "fileio.h"
#include <stdlib.h>
#include <cpm.h>

long fileio_get_modification_time(const char *filename) { return -1L; }

void fileio_set_modification_time(const char *filename, long mdate) {}

long get_file_size(FILE *fp) {
    fprintf(stderr, "Sorry, get_file_size() has not been implemented for CP/M.\n");
    exit(1);
    return -1;
}


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
