//
//  data_handler.c
//  Task_7_1
//
//  Created by Ольга Выростко on 24.04.17.
//  Copyright © 2017 Ольга Выростко. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include "launcher.defines.h"

enum DATA_HANDLER_ERROR_CODES {
    DATA_HANDLER_ERROR_OK,
    DATA_HANDLER_ERROR_HELP,
    DATA_HANDLER_ERROR_SELECT,
    DATA_HANDLER_ERROR_READ_OR_WRITE
};

size_t encodeChunk(char *buffer, size_t chunk_size, char *resultBuffer) {
    /*
     encoding of the chunk
     */
    int i = 0;
    for (i = 1; i < chunk_size; i++) {
        char bit = buffer[i] & 1;
        buffer[i] = bit + '0';
    }
    return chunk_size;
}

int main(int argc, char **argv) {
    /*
     handler: process chunk of data, encode it
     process char by char, replace each char with its low bit
     */
    
    if (argc < 3) {
        printf("Usage: ./data_handler src_descriptor dst_descriptor\n");
        return DATA_HANDLER_ERROR_HELP;
    }
    
    int src = atoi(argv[1]);
    int dst = atoi(argv[2]);
    
    while (true) {
        fd_set set_in;
        FD_ZERO(&set_in);
        FD_SET(src, &set_in);
        
        struct timeval timestamp;
        timestamp.tv_sec = 1;
        timestamp.tv_usec = 0;
        
        int selected;
        if (!(selected = select(src + 1, &set_in, NULL, NULL, &timestamp))) {
            break;
        }
        
        if (selected < 0) {
            printf("Failed to select src\n");
            return DATA_HANDLER_ERROR_SELECT;
        }
        
        char buffer[BUFFER_SIZE + 1];
        char resultBuffer[BUFFER_SIZE + 1];
        memset(resultBuffer, '\0', BUFFER_SIZE + 1);
        size_t read_bytes = read(src, &buffer, BUFFER_SIZE + 1);
        if (read_bytes) {
            printf("read from src: %s; offset: %hhd; read_bytes: %zu\n", &buffer[1], buffer[0], read_bytes);
            read_bytes = encodeChunk(buffer, read_bytes, resultBuffer);
            printf("%s\n", buffer);
            printf("read_bytes: %zu\n", read_bytes);
            
            fd_set set_out;
            FD_ZERO(&set_out);
            FD_SET(dst, &set_out);
            
            struct timeval out_timestamp;
            out_timestamp.tv_sec = 1;
            out_timestamp.tv_usec = 0;
            
            if (!(selected = select(dst + 1, NULL, &set_out, NULL, &out_timestamp))) {
                break;
            }
            
            if (selected < 0) {
                printf("Failed to select data dst\n");
                return DATA_HANDLER_ERROR_SELECT;
            }
            
            if (write(dst, &buffer, read_bytes) != read_bytes) {
                printf("Failed to write to destination\n");
                return DATA_HANDLER_ERROR_READ_OR_WRITE;
            }
        }
    }
    
    if ((close(src) < 0) || (close(dst) < 0)) {
        printf("Failed to close src or dst in handler\n");
        return DATA_HANDLER_ERROR_READ_OR_WRITE;
    }
    return 0;
}
