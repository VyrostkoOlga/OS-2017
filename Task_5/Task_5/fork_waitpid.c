//fork_waitpid.c
//Запускаем несколько дочерных процессов и следим за их завершением

#include <stdio.h>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <string.h>

#define MAXPROC 2
#define LINE_SIZE 255

enum OBSERVE_TYPES {
    OBSERVE_WAIT = 0,
    OBSERVE_RESTART = 1
};

struct ProcessInfo {
    char filename[LINE_SIZE];
    char args[LINE_SIZE];
    enum OBSERVE_TYPES type;
};

//массив статусов дочерних процессов
//чтобы не обнулять массив объявляем его внешним
pid_t pid_list[MAXPROC];
//на случай непредвиденных обстоятельств храним число процессов
int pid_count=0;
//массив информации о запускаемых процессах,
//считанный из конфигурационного файла
struct ProcessInfo pinfo_list[MAXPROC];

int readConfigFile(const char *filename) {
    FILE *fd = fopen(filename, "r");
    if (!fd) {
        printf("Error opening file\n");
        return -1;
    }
    
    int currentProc = 0;
    char execFilename[LINE_SIZE], execArgs[LINE_SIZE], mode[LINE_SIZE];
    while (currentProc < MAXPROC) {
        int read = fscanf(fd, "%s %s %s", execFilename, execArgs, mode);
        if (read == EOF) {
            break;
        }
        
        if (!read) {
            printf("Wrong input format\n");
            return -1;
        }
        memcpy(pinfo_list[currentProc].filename, execFilename, LINE_SIZE);
        memcpy(pinfo_list[currentProc].args, execArgs, LINE_SIZE);
        
        if (strcmp(mode, "wait") == 0) {
            pinfo_list[currentProc].type = OBSERVE_WAIT;
        } else if (strcmp(mode, "respawn") == 0) {
            pinfo_list[currentProc].type = OBSERVE_RESTART;
        } else {
            printf("Wrong input format\n");
            return -1;
        }
        currentProc++;
    }
    
    if (fclose(fd) < 0) {
        printf("Error while closing file\n");
        return -1;
    }
    
    return 0;
}

int startProcess(int p, pid_t cpid) {
    switch (cpid)
    {
        case -1:
            printf("Fork failed; cpid == -1\n");
            return -1;
        case 0:
            cpid = getpid();         //global PID
            
            char commandLine[LINE_SIZE * 2 + 1];
            memcpy(commandLine, pinfo_list[p].filename, LINE_SIZE);
            strcat(commandLine, " ");
            strcat(commandLine, pinfo_list[p].args);
            
            system(commandLine);
            printf("Exit child %d, pid = %d\n", p, cpid);
            exit(0);
            break;
        default:
            if (createPidFile(p, cpid) == -1) {
                printf("Could not write pid file\n");
            }
            
            pid_list[p]=cpid;
            pid_count++;
    }
    return 0;
}

int createPidFile(int p, pid_t cpid) {
    char filename[LINE_SIZE + 12];
    memcpy(filename, "/var/run/test", 8);
    //strcat(filename, pinfo_list[p].filename);
    strcat(filename, ".pid");
    
    FILE *fd = fopen(filename, "w");
    if (!fd) {
        printf("Error opening file\n");
        return -1;
    }
    
    if (fprintf(fd, "%d", cpid) < 0) {
        printf("Error writing pid file\n");
        return -1;
    }
    
    if (fclose(fd) < 0) {
        printf("Error closing file\n");
        return -1;
    }
    return 0;
}

/*
int main()
{
    if (readConfigFile("/Users/OlgaVyrostko/Documents/WorkMaterials/8sem/ОС/game/Task_5/Task_5/config") != 0) {
        printf("Error while reading config file\n");
        return -1;
    }
    
    int p;
    pid_t cpid;

    for (p=0; p<MAXPROC; p++)
    {
        cpid = fork();
        startProcess(p, cpid);
    }

    while (pid_count)
    {
            cpid=waitpid(-1, NULL, 0);   //ждем любого завершенного потомка
            for (p=0; p<MAXPROC; p++)
            {
                if(pid_list[p]==cpid)
                {
                    //делаем что-то по завершении дочернего процесса
                    printf("Child number %d pid %d finished\n",p,cpid);
                    pid_list[p]=0;
                    pid_count--;
                    
                    if (pinfo_list[p].type == OBSERVE_RESTART) {
                        cpid = fork();
                        startProcess(p, cpid);
                    }
                }
            }
    }

    return 0; 
}*/

