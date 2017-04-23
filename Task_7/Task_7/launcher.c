//
//  launcher.c
//  Task_7
//
//  Created by Ольга Выростко on 23.04.17.
//  Copyright © 2017 Ольга Выростко. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "launcher_defines.h"

struct Handler {
    int pid;
    int pipe[2];
    int result_pipe[2];
};

void processChunk(char *chunck, size_t chunck_size, char *resultChunck) {
    if (!chunck_size) {
        return;
    }
    
    int idx = 0;
    int resultIdx = 0;
    char len = 0;
    char currentSymbol = chunck[0];
    while (idx + len < chunck_size) {
        if ((chunck[idx + len] == currentSymbol) && (len < 9)) {
            len++;
            continue;
        }
        
        resultChunck[resultIdx++] = currentSymbol;
        resultChunck[resultIdx++] = len + '0';
        currentSymbol = chunck[++idx];
        len = 0;
    }
    resultChunck[resultIdx++] = currentSymbol;
    resultChunck[resultIdx++] = len + '0';
    resultChunck[resultIdx++] = '\0';
}

void handle(struct Handler handler) {
    printf("Child process\n");
    close(handler.pipe[1]);
    close(handler.result_pipe[0]);
    
    char cl_buffer[BUFFER_SIZE];
    char cl_resultBuffer[BUFFER_SIZE * 2];
    
    size_t cl_read_bytes, cl_write_bytes;
    
    cl_read_bytes = read(handler.pipe[0], cl_buffer, BUFFER_SIZE);
    processChunk(cl_buffer, cl_read_bytes, cl_resultBuffer);
    
    cl_write_bytes = write(handler.result_pipe[1], cl_resultBuffer, strlen(cl_resultBuffer));
    if (cl_write_bytes != strlen(cl_resultBuffer)) {
        printf("Failed to write result\n");
        exit(-1);
    }
}

int main(int argc, const char * argv[]) {
    
    char buffer[BUFFER_SIZE];
    char result_buffer[BUFFER_SIZE * 2];
    size_t read_bytes, write_bytes;
    
    FILE *rf = fopen(SRC_FILE, "r");
    if (!rf) {
        printf("Failed to open src file\n");
        return -1;
    }
    FILE *wf = fopen(DST_FILE, "w");
    if (!wf) {
        printf("Failed to open dst file\n");
        return -1;
    }
    
    struct Handler handler[MAX_HANDLERS_COUNT];
    
    pid_t pid;
    int p;
    for (p = 0; p < MAX_HANDLERS_COUNT; p++) {
        pipe(handler[p].pipe);
        pipe(handler[p].result_pipe);
        switch ((pid = fork())) {
            case -1:
                printf("Failed to fork\n");
                return -1;
            case 0:
                handle(handler[p]);
                exit(0);
            default:
                printf("Parent process\n");
                handler[p].pid = pid;
                close(handler[p].pipe[0]);
                close(handler[p].result_pipe[1]);
                
                while ((read_bytes = fread(buffer, sizeof(char), BUFFER_SIZE, rf)) > 0) {
                    write_bytes = write(handler[p].pipe[1], buffer, read_bytes);
                    if (write_bytes != read_bytes) {
                        printf("Failed to write data to pipe\n");
                        return -1;
                    }
                    read_bytes = read(handler[p].result_pipe[0], result_buffer, BUFFER_SIZE * 2);
                    write_bytes = fwrite(result_buffer, sizeof(char), read_bytes, wf);
                    if (write_bytes != read_bytes) {
                        printf("Failed to write data to file\n");
                        return -1;
                    }
                }
                
                close(handler[p].result_pipe[0]);
                close(handler[p].pipe[1]);
                
                exit(0);
        }
    }
    
    return 0;
}
