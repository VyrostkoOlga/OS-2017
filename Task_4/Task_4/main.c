//
//  main.c
//  Task_4
//
//  Created by Ольга Выростко on 08.04.17.
//  Copyright © 2017 Ольга Выростко. All rights reserved.
//

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
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
    
};

static const unsigned short FIELD_SIZE = 4;

static const unsigned char ONE_BIT = 1;
static const unsigned char TWO_BITS = 3;
static const unsigned char THREE_BITS = 7;

static const unsigned short STAY_ALIVE[2] = {2, 3};
static const unsigned short MAKE_ALIVE = 3;

static const int PORT = 8889;

unsigned char field[FIELD_SIZE];
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

void modifyRow(const unsigned char *fieldRow, unsigned char *currentRow, unsigned short column, int liveNear) {
    if (*fieldRow & (ONE_BIT << column)) {
        if ((liveNear == STAY_ALIVE[0]) || (liveNear == STAY_ALIVE[1])) {
            *currentRow |= (ONE_BIT << column);
        }
    } else if (liveNear == MAKE_ALIVE) {
        *currentRow |= (ONE_BIT << column);
    }
}

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
    
    return 0;
}

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

void printField(unsigned char *field) {
    for (unsigned short row = 0; row < FIELD_SIZE; row++) {
        for (unsigned short column = 0; column < FIELD_SIZE; column++) {
            printf("%d", (field[row] & (1 << column)) >> column);
        }
        printf("\n");
    }
}

void stopNextState(int sig) {
    printf("Error: next state is not counted after 1 second");
    if (close(socket_desc) < 0) {
        printf("Could not close server socket\n");
    }
    
    exit(-1);
}

void* processGame() {
    while (true) {
        alarm(1);
        nextState(field);
    }
}

int main(int argc, const char * argv[]) {
    signal(SIGALRM, stopNextState);
    
    if (initField("/Users/OlgaVyrostko/Documents/WorkMaterials/8sem/ОС/game/Task_4/Task_4/field", field) != INIT_FIELD_ERROR_OK) {
        return -1;
    }
    
    pthread_t gameId;
    if (pthread_create(&gameId, NULL, processGame, NULL)) {
        printf("Error creating game thread");
        return -1;
    }
    
    int client_sock, c;
    struct sockaddr_in server, client;
    
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create socket\n");
        return -1;
    }
    
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);
    
    if (bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
        printf("Could not bind\n");
        return -1;
    }
    
    if (listen(socket_desc, 10) < 0) {
        printf("Could not listen\n");
        return -1;
    }
    
    while ((client_sock = accept(socket_desc, (struct sockaddr *) &client, (socklen_t*)&c)) >= 0) {
        if (send(client_sock, field, FIELD_SIZE * FIELD_SIZE, 0) < 0) {
            printf("Could not write to client\n");
        }
        if (close(client_sock) < 0) {
            printf("Could not close connection with client\n");
        }
    }
    
    if (close(socket_desc) < 0) {
        printf("Could not close server socket\n");
        return -1;
    }
    
    return 0;
}
