//
//  client.c
//  Task_4
//
//  Created by Ольга Выростко on 08.04.17.
//  Copyright © 2017 Ольга Выростко. All rights reserved.
//

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

static const unsigned short FIELD_SIZE = 8;
static const int PORT = 8889;

enum ERROR_CODES {
    ERROR_CODE_OK = 0,
    ERROR_CODE_SOCKET_ERROR
};

void printField(unsigned char *field) {
    for (unsigned short row = 0; row < FIELD_SIZE; row++) {
        for (unsigned short column = 0; column < FIELD_SIZE; column++) {
            printf("%d", (field[row] & (1 << column)) >> column);
        }
        printf("\n");
    }
}

int main(int argc, const char * argv[]) {
    int sock;
    struct sockaddr_in server;
    unsigned char field[FIELD_SIZE * FIELD_SIZE];
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Could not create socket\n");
        return ERROR_CODE_SOCKET_ERROR;
    }
    
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Connection failed\n");
        return ERROR_CODE_SOCKET_ERROR;
    }
    
    if (recv(sock, field, FIELD_SIZE * FIELD_SIZE, 0) < 0) {
        printf("Receive failed\n");
        return ERROR_CODE_SOCKET_ERROR;
    }
    printField(field);
    
    if (close(sock) < 0) {
        printf("Close socket failed\n");
        return ERROR_CODE_SOCKET_ERROR;
    }
    return ERROR_CODE_OK;
}
