//
//  main.c
//  Task_4
//
//  Created by Ольга Выростко on 08.04.17.
//  Copyright © 2017 Ольга Выростко. All rights reserved.
//

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

enum INIT_FIELD_ERROR_CODES {
    INIT_FIELD_ERROR_OK = 0,
    INIT_FIELD_FILE_ERROR,
    INIT_FIELD_WRONG_INPUT
};

enum ERROR_CODES {
    ERROR_CODE_OK = 0,
    ERROR_CODE_INIT_FIELD_ERROR,
    ERROR_CODE_SOCKET_ERROR,
    ERROR_CODE_INTERRUPTION_ERROR,
    ERROR_CODE_THREAD_ERROR,
    ERROR_CODE_INSTALL_ALARM_ERROR
};

enum INSTALL_ALARM_ERROR_CODES {
    INSTALL_ALARM_OK = 0,
    INSTALL_ALARM_ERROR = -1
};

#define FIELD_SIZE 8

#define ONE_BIT 1
#define TWO_BITS 3
#define THREE_BITS 7

#define PORT 8889
#define INTERRUPTION_INTERVAL 1

unsigned char field[FIELD_SIZE];
bool finished;
bool handled;
int socket_desc;

int countLiveNear(unsigned char mask) {
    switch (mask) {
        case 0: return 0;
        case 1: return 1;
        case 2: return 1;
        case 3: return 2;
        case 4: return 1;
        case 5: return 2;
        case 6: return 2;
        case 7: return 3;
        default: return 0;
    }
}

// modify field row for cell at column according to alive neighbours and alive state
void modifyRow(const unsigned char *fieldRow, unsigned char *currentRow, unsigned short column, int liveNear) {
    if (*fieldRow & (ONE_BIT << column)) {
        if ((liveNear == 1) || (liveNear == 1)) {
            *currentRow |= (ONE_BIT << column);
        }
    } else if (liveNear == 3) {
        *currentRow |= (ONE_BIT << column);
    }
}

// count next state of the game
int nextState(unsigned char *field) {
    //sleep(1);
    
    unsigned char nextStateField[FIELD_SIZE];
    int liveNear;
    unsigned char mask, shiftColumn;
    
    for (unsigned short row = 0; row < FIELD_SIZE; row++) {
        nextStateField[row] ^= nextStateField[row];
        for (unsigned short column = 0; column < FIELD_SIZE; column++) {
            if (!column) {
                mask = TWO_BITS;
                shiftColumn = 0;
            } else if (column == FIELD_SIZE - 1) {
                mask = TWO_BITS;
                shiftColumn = -1;
            } else {
                mask = THREE_BITS;
                shiftColumn = -1;
            }
            
            liveNear = row ? countLiveNear((field[row - 1] & (mask << (column + shiftColumn))) >> (column + shiftColumn)) : 0;
            if (column) {
                liveNear += ((field[row] & (ONE_BIT << (column - 1))) >> (column - 1));
            }
            if (column != FIELD_SIZE - 1) {
                liveNear += ((field[row] & (ONE_BIT << (column + 1))) >> (column + 1));
            }
            if (row != FIELD_SIZE - 1) {
                liveNear += countLiveNear((field[row + 1] & (mask << (column + shiftColumn))) >> (column + shiftColumn));
            }
            modifyRow(&field[row], &nextStateField[row], column, liveNear);
        }
    }
    
    for (unsigned short row = 0; row < FIELD_SIZE; row++) {
        field[row] = nextStateField[row];
    }
    
    finished = true;
    return 0;
}

// read first game state from file if possible
int initField(const char *filename, unsigned char* field) {
    FILE *fd = fopen(filename, "r");
    if (!fd) {
        printf("Error opening file\n");
        return INIT_FIELD_FILE_ERROR;
    }
    
    int fsize = FIELD_SIZE * FIELD_SIZE + FIELD_SIZE;
    
    unsigned char buffer[fsize];
    if (fread(buffer, sizeof(char), fsize, fd) != fsize) {
        printf("Wrong input size (not enough information)\n");
        if (fclose(fd) < 0) {
            return INIT_FIELD_FILE_ERROR;
        }
        return INIT_FIELD_WRONG_INPUT;
    }
    
    unsigned char currentRow;
    for (unsigned short row = 0; row < FIELD_SIZE; row++) {
        currentRow = 0;
        for (unsigned short column = 0; column <= FIELD_SIZE; column++) {
            int index = row * (FIELD_SIZE + 1) + column;
            
            // ignore whitespace symbols
            if (iscntrl(buffer[index])) {
                continue;
            }
            
            if (buffer[index] == '0') {
                continue;
            }
            
            if (buffer[index] == '1') {
                currentRow |= (1 << column);
                continue;
            }
            
            printf("Wrong input format: found not 0, not 1\n");
            if (fclose(fd) < 0) {
                return INIT_FIELD_FILE_ERROR;
            }
            return INIT_FIELD_WRONG_INPUT;
        }
        field[row] = currentRow;
    }
    
    if (fclose(fd) < 0) {
        return INIT_FIELD_FILE_ERROR;
    }
    return INIT_FIELD_ERROR_OK;
}

// print field (on server for debug)
void printField(unsigned char *field) {
    for (unsigned short row = 0; row < FIELD_SIZE; row++) {
        for (unsigned short column = 0; column < FIELD_SIZE; column++) {
            printf("%d", (field[row] & (1 << column)) >> column);
        }
        printf("\n");
    }
}

// interruption handler (stop game because computation is longer than necessary)
void stopNextState(int sig) {
    if (!finished) {
        printf("Error: next state is not counted after 1 second");
        if (close(socket_desc) < 0) {
            printf("Could not close server socket\n");
        }
        
        exit(ERROR_CODE_INTERRUPTION_ERROR);
    }
    
    printf("handled\n");
    handled = true;
    finished = false;
}

// установка таймера с интервалом в 1 секунуд
int installAlarm() {
    struct sigaction sa;
    struct itimerval timer;
    memset(&sa, 0, sizeof (sa));
    sa.sa_handler = &stopNextState;
    sigaction(SIGALRM, &sa, NULL);
    timer.it_value.tv_sec = INTERRUPTION_INTERVAL;
    timer.it_value.tv_usec = 0;
    timer.it_interval = timer.it_value;
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        printf("Error setting timer\n");
        return INSTALL_ALARM_ERROR;
    }
    return INSTALL_ALARM_OK;
}

void* processGame() {
    printField(field);
    nextState(field);
    while (true) {
        if (handled) {
            printField(field);
            nextState(field);
            handled = false;
        }
    }
}

int main(int argc, const char * argv[]) {
    if (argc < 2) {
        printf("Not enough arguments\n");
        printf("Usage: ./main <file_with_initial_state>\n");
        printf("Initial state format: 8 rows of 8 elements, each row from new line\n");
        printf("0 = dead, 1 = live\n");
        printf("There shoud not be any symbols except 0, 1 and one \\n at the end of the line\n");
        return ERROR_CODE_OK;
    }
    
    // set alarm handler
    if (installAlarm() == INSTALL_ALARM_ERROR) {
        return ERROR_CODE_INSTALL_ALARM_ERROR;
    }
    finished = false;
    
    // start game thread
    pthread_t gameId;
    if (pthread_create(&gameId, NULL, processGame, NULL)) {
        printf("Error creating game thread");
        return ERROR_CODE_THREAD_ERROR;
    }
    
    // get initial state from file passed as the first non-preset command line argument
    if (initField(argv[1], field) != INIT_FIELD_ERROR_OK) {
        return ERROR_CODE_INIT_FIELD_ERROR;
    }
    
    // listen to clients' connections
    int client_sock, c;
    struct sockaddr_in server, client;
    
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create socket\n");
        return ERROR_CODE_SOCKET_ERROR;
    }
    
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);
    
    if (bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
        printf("Could not bind\n");
        return ERROR_CODE_SOCKET_ERROR;
    }
    
    if (listen(socket_desc, 10) < 0) {
        printf("Could not listen\n");
        return ERROR_CODE_SOCKET_ERROR;
    }
    
    while (true) {
        client_sock = accept(socket_desc, (struct sockaddr *) &client, (socklen_t*)&c);
        if (client_sock < 0) {
            // error while connection with client or alarm interruption
            // just ignore
            continue;
        }
        printf("Connected with client");
        printf("%d", client_sock);
        
        if (send(client_sock, field, FIELD_SIZE * FIELD_SIZE, 0) < 0) {
            printf("Could not write to client\n");
        }
        if (close(client_sock) < 0) {
            printf("Could not close connection with client\n");
        }
    }
    
    if (close(socket_desc) < 0) {
        printf("Could not close server socket\n");
        return ERROR_CODE_SOCKET_ERROR;
    }
    
    return ERROR_CODE_OK;
}
