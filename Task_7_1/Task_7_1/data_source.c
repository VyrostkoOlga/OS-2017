//
//  data_source.c
//  Task_7_1
//
//  Created by Ольга Выростко on 24.04.17.
//  Copyright © 2017 Ольга Выростко. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include <math.h>

#include "launcher.defines.h"

enum DATA_SOURCE_ERROR_CODES {
    DATA_SOURCE_ERROR_CODE_OK,
    DATA_SOURCE_ERROR_CODE_HELP,
    DATA_SOURCE_ERROR_CODE_WRONG_PARAMS,
    DATA_SOURCE_ERROR_CODE_FILE_ERROR,
    DATA_SOURCE_ERROR_CODE_SELECT_ERROR,
    DATA_SOURCE_ERROR_CODE_DEST_WRITE
};

int handlers_count = MAX_HANDLERS_COUNT;
struct DataDest {
    int fd;
} dests[MAX_HANDLERS_COUNT];

void initHandlers(char **argv) {
    /*
     init handlers from command line: need to get integers from strings
     */
    
    int handler_idx;
    for (handler_idx = 0; handler_idx < handlers_count; handler_idx++) {
        dests[handler_idx].fd = atoi(argv[handler_idx + 2]);
    }
}

int selectHandler(fd_set set) {
    int handler_idx;
    for (handler_idx = 0; handler_idx < handlers_count; handler_idx++) {
        if (FD_ISSET(dests[handler_idx].fd, &set)) {
            return handler_idx;
        }
    }
    return -1;
}

int main(int argc, char **argv) {
    /*
     data_source
     read src file in chunks of BUFFER_SIZE (from launcher.defines), send chunk by chunk to handlers
     params: src_file = path to src file
             dst_descriptor = file descriptor of dst (at most MAX_HANDLERS_COUNT)
     */
    
    if (argc < 3) {
        printf("usage: ./data_source src_file dst_descriptor[-s]\n");
        return DATA_SOURCE_ERROR_CODE_WRONG_PARAMS;
    }
    
    if ((!strcmp("-h", argv[1])) || (!strcmp("--help", argv[1]))) {
        printf("Usage: ./data_source src_file dst_descriptor[-s]\n");
        return DATA_SOURCE_ERROR_CODE_HELP;
    }
    
    handlers_count = argc - 2;
    if (handlers_count > MAX_HANDLERS_COUNT) {
        printf("Wrong input: number of handlers should be less than %d\n", MAX_HANDLERS_COUNT);
        return DATA_SOURCE_ERROR_CODE_WRONG_PARAMS;
    }
    
    if (handlers_count != 1) {
        handlers_count = handlers_count % 2 ? handlers_count - 1 : handlers_count;
    }
    
    initHandlers(argv);
    
    FILE *fd = fopen(argv[1], "r");
    if (!fd) {
        printf("Failed to open src file\n");
        return DATA_SOURCE_ERROR_CODE_FILE_ERROR;
    }
    
    size_t read_bytes;
    char buffer[BUFFER_SIZE + 1];
    memset(buffer, '\0', BUFFER_SIZE + 1);
    
    int i;
    int offset = 0;
    while ((read_bytes = fread(buffer, sizeof(char), BUFFER_SIZE, fd))) {
        printf("Read %zu bytes\n", read_bytes);
        size_t chunk_size = floor(read_bytes / handlers_count);
        size_t last_chunk_size = read_bytes - handlers_count * chunk_size;
        for (i = 0; i < handlers_count; i++) {
            if (i == handlers_count - 1) {
                chunk_size += last_chunk_size;
            }
            
            if (!chunk_size) {
                continue;
            }
            
            fd_set set;
            FD_ZERO(&set);
            FD_SET(dests[i].fd, &set);
            
            struct timeval timestamp;
            timestamp.tv_sec = 1;
            timestamp.tv_usec = 0;
            
            /*
             golden path: 
             if selected = 0 -> ok, break, 1 instr
             if selected < 0 -> error, process error, return
             if selected > 9 -> ok, continue, process chunk
             */
            
            int selected;
            if (!(selected = select(dests[i].fd + 1, NULL, &set, NULL, &timestamp))) {
                break;
            }
            
            if (selected < 0) {
                printf("Failed to select destination in data_source\n");
                return DATA_SOURCE_ERROR_CODE_SELECT_ERROR;
            }
            
            int inner_selected = selectHandler(set);
            printf("Selected %d\n", inner_selected);
            if (inner_selected < 0) {
                printf("Failed to select destination in data_source\n");
                return DATA_SOURCE_ERROR_CODE_SELECT_ERROR;
            }
            
            if (write(dests[inner_selected].fd, &offset, 1) != 1) {
                printf("Failed to write offset\n");
                return DATA_SOURCE_ERROR_CODE_DEST_WRITE;
            }
            
            if (write(dests[inner_selected].fd, &buffer[chunk_size * i], chunk_size) != chunk_size) {
                printf("Failed to write to destination\n");
                return DATA_SOURCE_ERROR_CODE_DEST_WRITE;
            }
            offset += chunk_size;
        }
    }

    if (fclose(fd) < 0) {
        printf("Failed to close src file\n");
        return DATA_SOURCE_ERROR_CODE_FILE_ERROR;
    }
    return DATA_SOURCE_ERROR_CODE_OK;
}
