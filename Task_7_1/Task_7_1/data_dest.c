//
//  data_dest.c
//  Task_7_1
//
//  Created by Ольга Выростко on 24.04.17.
//  Copyright © 2017 Ольга Выростко. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "launcher.defines.h"

enum DATA_DEST_ERROR_CODES {
    DATA_DEST_ERROR_CODE_OK,
    DATA_DEST_ERROR_CODE_HELP,
    DATA_DEST_ERROR_CODE_WRONG_PARAMS,
    DATA_DEST_ERROR_CODE_FILE_ERROR,
    DATA_DEST_ERROR_CODE_SELECT_ERROR,
    DATA_DEST_ERROR_CODE_DEST_WRITE
};

struct DataSrc {
    int fd;
} srcs[MAX_HANDLERS_COUNT];

int handlers_count = MAX_HANDLERS_COUNT;

void initHandlers(char ** argv) {
    int handler_idx;
    for (handler_idx = 0; handler_idx < handlers_count; handler_idx++) {
        srcs[handler_idx].fd = atoi(argv[handler_idx + 2]);
    }
}

int main(int argc, char **argv) {
    /*
     read data from handlers
     use buffer[0] to find position of data in dest file
     */
    
    if ((argc < 3) || (!strcmp("-h", argv[1])) || (!strcmp("--help", argv[1]))) {
        printf("Usage: ./data_dest destination_file src_descriptor[-s]\n");
        return DATA_DEST_ERROR_CODE_HELP;
    }
    
    FILE *dest = fopen(argv[1], "w");
    if (!dest) {
        printf("Failed to open destination file\n");
        return DATA_DEST_ERROR_CODE_FILE_ERROR;
    }
    
    handlers_count = argc - 2;
    if (handlers_count > MAX_HANDLERS_COUNT) {
        printf("Wrong params: number of handlers should be less than %d", MAX_HANDLERS_COUNT);
        return DATA_DEST_ERROR_CODE_WRONG_PARAMS;
    }
    
    initHandlers(argv);
    
    int j;
    char buffer[BUFFER_SIZE + 1];
    while (true) {
        fd_set set;
        FD_ZERO(&set);
        int handler_idx, fd_max = 0;
        for (handler_idx = 0; handler_idx < handlers_count; handler_idx++) {
            FD_SET(srcs[handler_idx].fd, &set);
            fd_max = fmaxf(srcs[handler_idx].fd, fd_max);
        }
        
        struct timeval timestamp;
        timestamp.tv_sec = 1;
        timestamp.tv_usec = 0;
        
        int selected;
        if (!(selected = select(fd_max + 1, &set, NULL, NULL, &timestamp))) {
            break;
        }
        
        if (selected < 0) {
            printf("Failed to select handler\n");
            return DATA_DEST_ERROR_CODE_SELECT_ERROR;
        }
        
        for (j = 0; j < handlers_count; j++) {
            if (FD_ISSET(srcs[j].fd, &set)) {
                size_t read_bytes = read(srcs[j].fd, buffer, BUFFER_SIZE + 1);
                printf("Offset: %d\n", buffer[0]);
                if (fseek(dest, buffer[0], SEEK_SET) < 0) {
                    printf("Failed to shift to offset in dest file\n");
                    return DATA_DEST_ERROR_CODE_FILE_ERROR;
                }
                if (fwrite(&buffer[1], sizeof(char), read_bytes - 1, dest) != read_bytes - 1) {
                    printf("Failed to write file\n");
                    return DATA_DEST_ERROR_CODE_FILE_ERROR;
                }
                break;
            }
        }
    }
    
    if (fclose(dest) < 0) {
        printf("Failed to close destination file\n");
        return DATA_DEST_ERROR_CODE_FILE_ERROR;
    }
    
    return DATA_DEST_ERROR_CODE_OK;
}
