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
    
    char buffer[BUFFER_SIZE];
    char resultBuffer[BUFFER_SIZE * 2];
    
    size_t read_bytes = read(handler.pipe[0], buffer, BUFFER_SIZE);
    processChunk(buffer, read_bytes, resultBuffer);
    
    FILE *fd = fopen(SRC_FILE, "w");
    if (!fd) {
        printf("Failed to open file to write");
        exit(-1);
    }
    
    if (!fwrite(resultBuffer, sizeof(char), strlen(resultBuffer), fd)) {
        printf("Failed to write file\n");
        exit(-1);
    }
    
    if (fclose(fd) < 0) {
        printf("Failed to close file\n");
        exit(-1);
    }
}

int main(int argc, const char * argv[]) {
    
    //char buffer[BUFFER_SIZE];
    
    /*
    FILE *fd = fopen(SRC_FILE, "r");
    size_t read_bytes;
    while ((read_bytes = fread(buffer, sizeof(char), BUFFER_SIZE, fd)) > 0) {
        
    }*/
    
    struct Handler handler;
    pipe(handler.pipe);
    
    pid_t pid;
    switch ((pid = fork())) {
        case -1:
            printf("Failed to fork\n");
            return -1;
        case 0:
            handle(handler);
            exit(0);
        default:
            printf("Parent process\n");
            close(handler.pipe[0]);
            write(handler.pipe[1], "taaabb", 6);
            exit(0);
    }
    
    return 0;
}
