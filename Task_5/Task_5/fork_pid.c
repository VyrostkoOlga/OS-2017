//
//  main.c
//  Task_5
//
//  Created by Ольга Выростко on 22.04.17.
//  Copyright © 2017 Ольга Выростко. All rights reserved.
//

#include "fork_pid_defines.h"

struct Process* pinfo_list[MAX_PROCESSES_COUNT];            // processes that are observed
int process_count = 0;                                      // total number of processes loaded from config
int active_process_count = 0;                               // total number of active processes (started without errors, not blocked)

bool reload_flag = true;                                    // set when HUP is received
bool check_awake_flag = false;                              // set when one second is elapsed

void closeAndProcessError(FILE* fd) {
    if (fclose(fd) < 0) {
        syslog(LOG_ERR, "Failed to close file");
    }
}

void closeIntdAndProcessError(int fd) {
    if (close(fd) < 0) {
        syslog(LOG_ERR, "Failed to close file");
    }
}

char **strsplit(const char* str, const char* delim, size_t* numtokens) {
    char *s = strdup(str);
    size_t tokens_alloc = 1;
    size_t tokens_used = 0;
    char **tokens = calloc(tokens_alloc, sizeof(char*));
    char *token, *strtok_ctx;
    for (token = strtok_r(s, delim, &strtok_ctx);
         token != NULL;
         token = strtok_r(NULL, delim, &strtok_ctx)) {
        if (tokens_used == tokens_alloc) {
            tokens_alloc *= 2;
            tokens = realloc(tokens, tokens_alloc * sizeof(char*));
        }
        tokens[tokens_used++] = strdup(token);
    }
    if (tokens_used == 0) {
        free(tokens);
        tokens = NULL;
    } else {
        tokens = realloc(tokens, tokens_used * sizeof(char*));
    }
    *numtokens = tokens_used;
    free(s);
    return tokens;
}

void configure() {
    /*
     read initial configuration from config file
     read line by line
     each line should be: filename args mode, separator is one space
     mode should be wait or respawn
     */
    
    syslog(LOG_INFO, "Start configuration");
    FILE *fd = fopen(CONFIG_FILE, "r");
    if (!fd) {
        syslog(LOG_ERR, "Failed to read configuration file %s", CONFIG_FILE);
        return;
    }
    
    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    process_count = 0;
    
    while ((process_count < MAX_PROCESSES_COUNT) && (linelen = getline(&line, &linecap, fd) > 0)) {
        char *initial_line = line;
        
        struct Process *proc = malloc(sizeof(struct Process));
        memset(proc, 0, sizeof(struct Process));
        
        size_t argc;
        char **argv = strsplit(line, " ", &argc);
        
        // need at least: filename + mode
        if (argc < 2) {
            syslog(LOG_WARNING, "Failed to parse config line: %s", initial_line);
            free(initial_line);
            line = NULL;
            continue;
        }
        
        if (strcmp(argv[argc - 1], "wait\n") == 0) {
            proc->mode = OBSERVE_WAIT;
        } else if (strcmp(argv[argc - 1], "respawn\n") == 0) {
            proc->mode = OBSERVE_RESPAWN;
        } else {
            syslog(LOG_WARNING, "Failed to parse config line: %s", initial_line);
            free(initial_line);
            line = NULL;
            continue;
        }
        
        proc->filename = argv[0];
        int i;
        for (i=0; i<MIN(argc - 1, MAX_ARGS_COUNT); i++) {
            proc->commandLine[i] = argv[i];
        }
        
        pinfo_list[process_count++] = proc;
        line = NULL;
    }
    
    closeAndProcessError(fd);
}

void processFailureRun(struct Process *proc) {
    /*
     fail to start one of processes
     need to check if should restart or block process
     */
    
    proc->failures++;
    if (proc->failures > MAX_FAILURES) {
        syslog(LOG_ERR, "One of processes could not be launched");
        proc->failures = 0;
        proc->block_time = time(NULL);
    }
}

int createPidFile(int p, pid_t cpid) {
    /*
     create .pid file for the process
     for the filename use template and process id
     */
    
    char filename[PID_FILENAME_LENGHT];
    sprintf(filename, PID_FILE_PATTERN, p);
    
    FILE *fd = fopen(filename, "w");
    if (fd == NULL) {
        return -1;
    }
    
    fprintf(fd, "%d\n", cpid);
    closeAndProcessError(fd);
    return 0;
}

void startProcess(int p) {
    /*
     start process with index p (p-th line from config file)
     fork current process, for child = start p-th process, for parent: save child's info
     */
    
    pid_t cpid = fork();
    switch (cpid) {
        case FORK_ERROR:
            syslog(LOG_ERR, "Failed to fork the parent process for %s", pinfo_list[p]->filename);
            processFailureRun(pinfo_list[p]);
            break;
        case FORK_CHILD: {
            execv((&(pinfo_list[p])->filename)[0], (pinfo_list[p])->commandLine);
            syslog(LOG_ERR, "Failed to start %s", pinfo_list[p]->filename);
            exit(1);
        }
        default:
            syslog(LOG_INFO, "Start process %d", cpid);
            active_process_count++;
            pinfo_list[p]->pid = cpid;
            
            if (createPidFile(p, cpid) == -1) {
                syslog(LOG_ERR, "Failed to create pid file %d", p);
            } else {
                syslog(LOG_INFO, "Successfully created pid file %d", p);
            }
    }
}

void reload() {
    /*
     reload initial configuration (reload signal handled, maybe initial config file was changed)
     */
    
    syslog(LOG_INFO, "Received reload signal, need to reload initial data");
    
    // first of all finish current observation process finishing all subprocesses
    int p;
    for (p = 0; p < process_count; p++) {
        if (!pinfo_list[p]->pid) {
            continue;
        }
        if (kill(pinfo_list[p]->pid, SIGKILL) < 0) {
            syslog(LOG_ERR, "Failed to kill process");
        }
        active_process_count--;
    }
    
    // then restart
    configure();
    for (p = 0; p < process_count; p++) {
        startProcess(p);
    }
    reload_flag = false;
}

void timerTickDidReceived(int signum) {
    check_awake_flag = true;
}

int installTimer() {
    struct sigaction sa;
    struct itimerval timer;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &timerTickDidReceived;
    sigaction(SIGALRM, &sa, NULL);
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    timer.it_interval = timer.it_value;
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        syslog(LOG_ERR, "Failed to set timer");
        return -1;
    }
    return 0;
}

void reloadSignalDidReceived(int signum) {
    syslog(LOG_INFO, "Reload signal received");
    reload_flag = true;
}

void installReloadSignal() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &reloadSignalDidReceived;
    sigaction(SIGHUP, &sa, NULL);
}

void processRestartProcess(int p) {
    /*
     check if could restart process: it is not is wait mode, it is not blocked
     if need to restart, restart
     */
    
    if (pinfo_list[p]->mode == OBSERVE_WAIT || pinfo_list[p]->mode == OBSERVE_NONE) {
        syslog(LOG_INFO, "Process %s should be executed only once, finish", pinfo_list[p]->filename);
        return;
    }
    
    if (pinfo_list[p]->block_time) {
        syslog(LOG_INFO, "Process %s blocked, should not be restarted", pinfo_list[p]->filename);
        return;
    }
    
    syslog(LOG_INFO, "Restart process %s", pinfo_list[p]->filename);
    startProcess(p);
}

void processCommandLineWithTwoArgs(char *arg) {
    /*
     program was started with two arguments:
     - if -d = should start as daemon
     - if -h, --help = should print help
     - else - wrong argument, print help
     */
    
    if (!strcmp(arg, "-h") || !strcmp(arg, "--help") || strcmp(arg, "-d")) {
        printf("Usage: ./fork_pid [-d]\n");
        printf("Use -d to start as daemon\n");
        printf("Config file should have path: %s", CONFIG_FILE);
        exit(0);
    }
    
    // executed with -d = start as daemon
    int fd;
    struct rlimit flim;
    
    if (getppid() != 1) {
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        
        switch (fork()) {
            case 0:
                setsid();
                break;
            default:
                exit(0);
        }
    }
    
    // close all file descriptors
    getrlimit(RLIMIT_NOFILE, &flim);
    for (fd = 0; fd < flim.rlim_max; fd++) {
        closeIntdAndProcessError(fd);
    }
    
    // switch to root dir
    if (chdir("/") < 0) {
        syslog(LOG_ERR, "Failed to switch to root dir");
    }
    
    syslog(LOG_INFO, "fork_pid daemon Launched");
}

void setupInterruptions() {
    installTimer();
    installReloadSignal();
}

int main(int argc, char ** argv) {
    // preparations
    openlog("fork_pid", LOG_PID | LOG_CONS, LOG_DAEMON);
    if (argc == 2) {
        processCommandLineWithTwoArgs(argv[1]);
    }
    setupInterruptions();
    
    int i = 0;
    // main cycle
    while (true) {
        if (reload_flag) {
            reload_flag = false;
            reload();
        }
        
        int p, status;
        if (check_awake_flag) {
            // need to check is there are processes which should not be blocked anymore (one second elapsed)
            for (p = 0; p < process_count; p++) {
                if (!pinfo_list[p]->block_time) {
                    continue;
                }
                
                if (time(NULL) - pinfo_list[p]->block_time <= BLOCK_TIMEOUT) {
                    continue;
                }
                
                // need to awake
                syslog(LOG_INFO, "Awake after block %s", pinfo_list[p]->filename);
                pinfo_list[p]->block_time = 0;
                processRestartProcess(p);
            }
            check_awake_flag = false;
        }
        
        
        // wait for one of the processes to finish
        pid_t cpid = waitpid(-1, &status, 0);
        for (p = 0; p < process_count; p++) {
            if (pinfo_list[p]->pid != cpid) {
                continue;
            }
            
            // WIFEXITED(status) returns a nonzero value if the child process terminated normally with exit or _exit.
            if (WIFEXITED(status)) {
                // WEXITSTATUS(status) returns the low-order 8 bits of the exit status value from the child process; if non-zero, add one more failure for process
                if (WEXITSTATUS(status)) {
                    processFailureRun(pinfo_list[p]);
                } else {
                    pinfo_list[p]->failures = 0;
                }
            }
            
            char filename[PID_FILENAME_LENGHT];
            sprintf(filename, PID_FILE_PATTERN, p);
            if (unlink(filename) < 0) {
                syslog(LOG_INFO, "Failed to unlink pid file: %d", cpid);
            }
            
            syslog(LOG_INFO, "Child process %d terminated", cpid);
            active_process_count--;
            processRestartProcess(p);
            break;
        }
        
        i++;
        if (i == 1) {
            reload_flag = true;
            i++;
        }
        if (!active_process_count && !reload_flag) {
            break;
        }
    }
    
    closelog();
    return 0;
}
