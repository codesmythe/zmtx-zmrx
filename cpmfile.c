

#include "fileio.h"
#include <stdlib.h>
#include <cpm.h>
#include <stdio.h>

long fileio_get_modification_time(const char *filename) { return -1L; }

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
