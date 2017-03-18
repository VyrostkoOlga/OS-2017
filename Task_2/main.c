//
//  main.c
//  Sparse-File-C
//
//  Created by Ольга Выростко on 12.03.17.
//  Copyright © 2017 Ольга Выростко. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

static int const FILE_PERMISSIONS_RWRR = 0644;
static int const FILE_OP_ERROR_INDICATOR = -1;

static int const BUFFER_SIZE = 255;
static int const STDIN = 0;

enum ERROR_CODE {
    ERROR_CODE_SUCCESS = 0,
    ERROR_CODE_NOT_ENOUGH_ARGUMENTS,
    ERROR_CODE_FILE_OPEN_ERROR,
    ERROR_CODE_FILE_CLOSE_ERROR,
    ERROR_CODE_FILE_READ_ERROR,
    ERROR_CODE_FILE_WRITE_ERROR,
    ERROR_CODE_FILE_LSEEK_ERROR
};

int erase_zeros(int fd) {
    char buffer[BUFFER_SIZE];
    long read_bytes, count;
    bool endZero = false;
    
    while ((read_bytes = read(STDIN, buffer, BUFFER_SIZE))) {
        if (read_bytes == FILE_OP_ERROR_INDICATOR) {
            printf("Error while trying to read stdin\n");
            return ERROR_CODE_FILE_READ_ERROR;
        }
        
        // read new BUFFER_SIZE-chunck of file
        long idx = 0;
        endZero = false;
        while (idx < read_bytes) {
            // write non-zero chunck from buffer
            
            count = 0;
            while (buffer[idx] && idx++ < read_bytes) { count++; }
            if (write(fd, buffer + idx - count, count) == FILE_OP_ERROR_INDICATOR) {
                printf("Error while trying to write file\n");
                return ERROR_CODE_FILE_WRITE_ERROR;
            }
            
            // lseek zero chunck
            count = 0;
            while (!buffer[idx] && idx++ < read_bytes) { count++; };
            if (lseek(fd, count, SEEK_CUR) == FILE_OP_ERROR_INDICATOR) {
                printf("Error while trying to lseek\n");
                return ERROR_CODE_FILE_LSEEK_ERROR;
            }
            
            if (idx >= read_bytes) {
                // indicate that chunk ends with zero,
                // if end of input -> need to write one
                // zero
                endZero = true;
            }
        }
    }
    if (endZero) {
        // file ends with zero, need to write one zero at the end
        // of file
        if (lseek (fd, -1, SEEK_CUR) == FILE_OP_ERROR_INDICATOR) {
            printf("Error while trying to lseek\n");
            return ERROR_CODE_FILE_LSEEK_ERROR;
        }
        if (write(fd, buffer + read_bytes - 1, 1) == FILE_OP_ERROR_INDICATOR) {
            printf("Error while writing file\n");
            return ERROR_CODE_FILE_WRITE_ERROR;
        }
    }
    
    return 0;
}

int main(int argc, const char * argv[]) {
    
    if (argc < 2) {
        printf("Not enough arguments\n");
        printf("Usage: input | %s filename\n", argv[0]);
        return ERROR_CODE_NOT_ENOUGH_ARGUMENTS;
    }
    
    int fd = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, FILE_PERMISSIONS_RWRR);
    if (fd == FILE_OP_ERROR_INDICATOR) {
        printf("Could not open file\n");
        return ERROR_CODE_FILE_OPEN_ERROR;
    }
    
    int ret;
    if ((ret = erase_zeros(fd)) == FILE_OP_ERROR_INDICATOR) {
        printf("Could not sparse this file\n");
        return ret;
    }
    if (close(fd) == FILE_OP_ERROR_INDICATOR) {
        printf("Could not close the file\n");
        return ERROR_CODE_FILE_CLOSE_ERROR;
    }
    
    printf("Done\n");
    return ERROR_CODE_SUCCESS;
}
