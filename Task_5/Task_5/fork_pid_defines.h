//
//  fork_pid_defines.h
//  Task_5
//
//  Created by Ольга Выростко on 22.04.17.
//  Copyright © 2017 Ольга Выростко. All rights reserved.
//

#ifndef fork_pid_defines_h
#define fork_pid_defines_h

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>

#define CONFIG_FILE "/etc/fork_pid_config"
#define PID_FILE_PATTERN "/tmp/fork_pid_%d.pid"
#define PID_FILENAME_LENGHT 30

#define MAX_PROCESSES_COUNT 10
#define MAX_FAILURES 50
#define BLOCK_TIMEOUT 5

enum OBSERVE_MODES {
    OBSERVE_WAIT = 0,
    OBSERVE_RESPAWN,
    OBSERVE_NONE
};

enum FORK_CONSTANTS {
    FORK_ERROR = -1,
    FORK_CHILD = 0
};

#define MAX_ARGS_COUNT 20

struct Process {
    char *filename;
    char *commandLine[MAX_ARGS_COUNT];
    enum OBSERVE_MODES mode;
    
    pid_t pid;
    int failures;
    time_t block_time;
};

#endif /* fork_pid_defines_h */
