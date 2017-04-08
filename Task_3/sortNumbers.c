//
//  main.c
//  SortNumbers
//
//  Created by Ольга Выростко on 04.04.17.
//  Copyright © 2017 Ольга Выростко. All rights reserved.
//

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>

enum INSERT_NODE_ERROR_CODES {
    INSERT_NODE_ERROR_OK = 0,
    INSERT_NODE_ERROR_CREATE_NODE = -1
};

enum WRITE_LIST_TO_FILE_ERROR_CODES {
    WRITE_LIST_TO_FILE_ERROR_OK = 0,
    WRITE_LIST_TO_FILE_ERROR_EMPT_LIST,
    WRITE_LIST_TO_FILE_ERROR_FILE_OP
};

enum ERROR_CODES {
    ERROR_CODE_OK = 0,
    ERROR_CODE_MEMORY_ERROR,
    ERROR_CODE_FILE_OP_ERROR
};

struct Node {
    float number;
    struct Node* next;
};

struct Node* createNode(long number) {
    struct Node* newN = malloc(sizeof(struct Node));
    if (!newN) {
        printf("Failed to allocate memory");
        return NULL;
    }
    newN->number = number;
    return newN;
}

int insertNode(struct Node *head, long number) {
    struct Node* newN = malloc(sizeof(struct Node));
    if (!newN) {
        printf("Failed to allocate memory");
        return INSERT_NODE_ERROR_CREATE_NODE;
    }
    newN->number = number;
    
    struct Node* current = head;
    while ((current->next) && (current->next->number >= number)) {
        current = current->next;
    }
    struct Node* tmp = current->next;
    current->next = newN;
    newN->next = tmp;
    return INSERT_NODE_ERROR_OK;
}

void printList(struct Node* head) {
    struct Node* current = head;
    if ((!head) || (current->next == current)) {
        return;
    }
    
    while (current) {
        printf("%f\n", current->number);
        current = current->next;
    }
}

int writeListToFile(const char *filename, struct Node* head) {
    if ((!head) || (head->next == head)) {
        return WRITE_LIST_TO_FILE_ERROR_EMPT_LIST;
    }
    
    FILE* fd = fopen(filename, "w+");
    if (!fd) {
        return WRITE_LIST_TO_FILE_ERROR_FILE_OP;
    }
    
    struct Node* current = head;
    while (current) {
        if (fprintf(fd, "%.02f\n", current->number) < 0) {
            return WRITE_LIST_TO_FILE_ERROR_FILE_OP;
        }
        current = current->next;
    }
    
    return WRITE_LIST_TO_FILE_ERROR_OK;
}

void freeList(struct Node* head) {
    struct Node *current = head;
    struct Node *next = head->next;
    while (next) {
        free(current);
        current = next;
        next = current->next;
    }
    free(current);
}

int main(int argc, const char * argv[]) {
    
    struct Node *head = malloc(sizeof(struct Node));
    if (!head) {
        printf("Failed to allocate memory\n");
        return ERROR_CODE_MEMORY_ERROR;
    }
    head->next = head;
    
    float currentNum;
    long readNumbers = 0;
    for (long idx = 1; idx < argc; idx++) {
        
        FILE *fd = fopen(argv[idx], "r");
        if (!fd) {
            printf("Error opening file. Could continue with other files\n");
            continue;
        }
        
        while ((readNumbers = fscanf(fd, "%f", &currentNum)) != EOF) {
            if (!readNumbers) {
                getc(fd);
                continue;
            }
            
            if (head->next == head) {
                head->number = currentNum;
                head->next = NULL;
                continue;
            }
            
            if (currentNum > head->number) {
                struct Node* newN = createNode(currentNum);
                if (!newN) {
                    printf("Error allocating memory. Fatal error.\n");
                    freeList(head);
                    return ERROR_CODE_MEMORY_ERROR;
                }
                newN->next = head;
                head = newN;
                continue;
            }
            
            if (insertNode(head, currentNum) == INSERT_NODE_ERROR_CREATE_NODE) {
                printf("Error insert node. Possible memory error. Fatal error.\n");
                freeList(head);
                return ERROR_CODE_MEMORY_ERROR;
            }
        }
        if (fclose(fd) < 0) {
            printf("Error closing file. Fatal error.\n");
            return ERROR_CODE_FILE_OP_ERROR;
        }
    }
    
    if (writeListToFile(argv[argc - 1], head) == WRITE_LIST_TO_FILE_ERROR_FILE_OP) {
        freeList(head);
        return ERROR_CODE_FILE_OP_ERROR;
    }
    
    freeList(head);
    return ERROR_CODE_OK;
}
