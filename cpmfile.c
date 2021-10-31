

#include "fileio.h"
#include <stdlib.h>
#include <stdio.h>

long fileio_get_modification_time(const char *filename) { return -1L; }

void fileio_set_modification_time(const char *filename, long mdate) {}

long get_file_size(FILE *fp) {
    fprintf(stderr, "Sorry, get_file_size() has not been implemented for CP/M.\n");
    exit(1);
    return -1;
}

/* remove comments to implement get_file_size() function

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
  
    // closing the file
    fclose(fp);
  
    return res;
}

*/
