//
//  launcher.c
//  Task_7_1
//
//  Created by Ольга Выростко on 24.04.17.
//  Copyright © 2017 Ольга Выростко. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "launcher.defines.h"

enum LAUNCHER_ERROR_CODES {
    LAUNCHER_ERROR_OK,
    LAUNCHER_ERROR_HELP,
    LAUNCHER_ERROR_WRONG_PARAMS,
    LAUNCHER_ERROR_FORK
};

int handlers_count = MAX_HANDLERS_COUNT;

struct HandlerPipe {
    int handler;
    int src[2];
    int dst[2];
} pipes[MAX_HANDLERS_COUNT];

char *formatProcessLine(int p) {
    char *line = (char *) malloc(32);
    sprintf(line, "%d", p);
    return line;
}

void fillHandlerArgs(char *hargs[4], int idx) {
    hargs[0] = HANDLER_EXC;
    hargs[1] = formatProcessLine(pipes[idx].src[0]);
    hargs[2] = formatProcessLine(pipes[idx].dst[1]);
    hargs[3] = 0;
}

void closeAll(int spid, int dpid) {
    int i;
    waitpid(spid, NULL, 0);
    for (i = 0; i < handlers_count; i++) {
        close(pipes[i].src[0]);
    }
    for (i = 0; i < handlers_count; i++) {
        waitpid(pipes[i].handler, NULL, 0);
        close(pipes[i].src[1]);
        close(pipes[i].dst[0]);
    }
    waitpid(dpid, NULL, 0);
    for (i = 0; i < handlers_count; i++) {
        close(pipes[i].dst[1]);
    }
}

int main(int argc, char **argv) {
    
    if (argc != 4) {
        printf("Usage: ./launcher src_file dst_file number_of_handlers\n");
        return LAUNCHER_ERROR_WRONG_PARAMS;
    }
    
    if ((!strcmp("-h", argv[1])) || (!strcmp("--help", argv[1]))) {
        printf("Usage: ./launcher src_file dst_file number_of_handlers\n");
        return LAUNCHER_ERROR_HELP;
    }
    
    handlers_count = atoi(argv[3]);
    if (handlers_count > MAX_HANDLERS_COUNT) {
        printf("Wrong params: number of handlers should be less than %d", MAX_HANDLERS_COUNT);
        return LAUNCHER_ERROR_WRONG_PARAMS;
    }
    
    if (handlers_count != 1) {
        handlers_count = handlers_count % 2 ? handlers_count - 1 : handlers_count;
    }
    
    // create pipes
    int i;
    for (i = 0; i < handlers_count; i++) {
        if (pipe(pipes[i].src) == -1) {
            perror("Error creating pipe from src to hdl");
            return 1;
        }
        if (pipe(pipes[i].dst) == -1) {
            perror("Error creating pipe from hdl to dst");
            return 1;
        }
    }
    
    char *sargs[handlers_count + 3];
    char *dargs[handlers_count + 3];
    int spid, dpid;
    
    sargs[0] = SRC_EXC;
    dargs[0] = DEST_EXC;
    sargs[1] = argv[1];
    dargs[1] = argv[2];
    for (i = 0; i < handlers_count; i++) {
        sargs[i + 2] = formatProcessLine(pipes[i].src[1]);
        dargs[i + 2] = formatProcessLine(pipes[i].dst[0]);
    }
    sargs[handlers_count + 2] = dargs[handlers_count + 2] = 0;
    
    switch ((spid = fork())) {
        case -1:
            printf("Failed to fork process for src\n");
            return LAUNCHER_ERROR_FORK;
        case 0:
            execv(sargs[0], sargs);
            printf("Failed to fork process for src\n");
            return LAUNCHER_ERROR_FORK;
        default:
            break;
    }
    switch (dpid = fork()) {
        case -1:
            printf("Failed to fork process for dst\n");
            return LAUNCHER_ERROR_FORK;
        case 0:
            execv(dargs[0], dargs);
            printf("Failed to fork process for dst\n");
            return LAUNCHER_ERROR_FORK;
        default:
            break;
    }
    
    for (i = 0; i < handlers_count; i++) {
        char *hargs[4];
        fillHandlerArgs(hargs, i);
        
        switch ((pipes[i].handler = fork())) {
            case -1:
                printf("Failed to fork process for handler\n");
                return LAUNCHER_ERROR_FORK;
            case 0:
                execv(hargs[0], hargs);
                printf("Failed to fork process for handler\n");
                return LAUNCHER_ERROR_FORK;
            default:
                break;
        }
    }
    
    closeAll(spid, dpid);
}
