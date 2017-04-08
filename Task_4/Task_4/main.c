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

enum INIT_FIELD_ERROR_CODES {
    INIT_FIELD_ERROR_OK = 0,
    INIT_FIELD_FILE_ERROR,
    INIT_FIELD_WRONG_INPUT
};

static const unsigned short FIELD_SIZE = 8;

static const unsigned char ONE_BIT = 1;
static const unsigned char TWO_BITS = 3;
static const unsigned char THREE_BITS = 7;

static const unsigned short STAY_ALIVE[2] = {2, 3};
static const unsigned short MAKE_ALIVE = 3;

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
            *currentRow |= ONE_BIT;
        }
    } else if (liveNear == MAKE_ALIVE) {
        *currentRow |= ONE_BIT;
    }
}

int nextState(unsigned char *field) {
    unsigned char nextStateField[FIELD_SIZE];
    
    int liveNear;
    unsigned char currentRow = 0;
    
    // 0 row
    liveNear = ((field[0] & (ONE_BIT << 1)) >> 1);
    liveNear += countLiveNear(field[1] & TWO_BITS);
    modifyRow(&field[0], &currentRow, 0, liveNear);
    
    for (unsigned short row = 1; row < FIELD_SIZE - 1; row++) {
        currentRow = 0;
        
        // 0 column
        liveNear = countLiveNear(field[row - 1] & TWO_BITS);
        liveNear += ((field[row] & (ONE_BIT << 1)) >> 1);
        liveNear += countLiveNear(field[row + 1] & TWO_BITS);
        modifyRow(&field[row], &currentRow, 0, liveNear);
        
        for (unsigned short column = 1; column < FIELD_SIZE - 1; column++) {
            
        }
        
        // last column
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

int main(int argc, const char * argv[]) {
    unsigned char field[FIELD_SIZE];
    
    if (initField("/Users/OlgaVyrostko/Documents/WorkMaterials/8sem/ОС/game/Task_4/Task_4/field", field) != INIT_FIELD_ERROR_OK) {
        return -1;
    }
    printField(field);
    nextState(field);
    printf("New\n");
    printField(field);
    
    return 0;
}
