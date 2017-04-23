//
//  lock_file.c
//  Task_6
//
//  Created by Ольга Выростко on 23.04.17.
//  Copyright © 2017 Ольга Выростко. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

enum LOCK_TYPES {
    LOCK_READ = 0,
    LOCK_WRITE
};

#define LOCK_FILE_EXT ".lck"
#define MAX_LOCK_FILENAME_LENGTH 255
#define MAX_NUMBER_OF_LOCKS 20
#define LOCK_LINE_PATTERN "%d %d %d %d\n"
#define LOCK_LOCK_LINE_PATTERN "%d %d\n"

#define WAIT_TIMEOUT 5
#define MAX_NUMBER_OF_TRIES 50

#define BUFFER_SIZE 50
#define TEST_FILENAME "./passwords"

struct Lock {
    int pid;                                // identifier of process that locks
    enum LOCK_TYPES lock;                   // type of lock (read/write)
    
    int offset;                             // offset in file
    int limit;                              // number of bytes after offset
};

bool conflictLockTypes(enum LOCK_TYPES lock1, enum LOCK_TYPES lock2) {
    /*
     read-read is not a conflict, all others - conflict
     */
    if (lock1 == LOCK_WRITE) { return true; }
    
    return (lock2 == LOCK_WRITE);
}

bool hasIntersection(int foffset, int flimit, int soffset, int slimit) {
    /*
     check if there is intersection in file's parts
     */
    
    if ((soffset >= foffset) && (soffset < foffset + flimit)) {
        return true;
    }
    
    if ((foffset >= soffset) && (foffset < soffset + slimit)) {
        return true;
    }
    return false;
}

bool checkLockLockFile(char *lockFilename) {
    /*
     check if lockLock file exists; if exists, lock
     file could not be modified
     */
    
    FILE *fd = fopen(lockFilename, "r");
    if (!fd) {
        printf("Lock for lock does not exist, create lock\n");
        return false;
    }
    
    if (fclose(fd) < 0) {
        printf("Failed to close lock lock file\n");
    }
    return true;
}

int createLockLock(char *filename, enum LOCK_TYPES lock) {
    /*
     create lock file for the lock file if it's possible
     if lock lock file has already been created, wait
     */
    
    char lockFilename[MAX_LOCK_FILENAME_LENGTH + strlen(LOCK_FILE_EXT)];
    memset(lockFilename, '\0', MAX_LOCK_FILENAME_LENGTH);
    memcpy(lockFilename, filename, strlen(filename));
    strcat(lockFilename, LOCK_FILE_EXT);
    
    while (checkLockLockFile(lockFilename)) {
        printf("Wait for lock lock creation\n");
        sleep(WAIT_TIMEOUT);
    }
    
    printf("Create lock lock %s", lockFilename);
    FILE * fd = fopen(lockFilename, "w");
    if (!fd) {
        printf("Failed to create lock lock file\n");
        return -1;
    }
    
    if (fprintf(fd, LOCK_LOCK_LINE_PATTERN, getpid(), lock) < 0) {
        printf("Failed to write lock lock file\n");
        return -1;
    }
    
    if (fclose(fd) < 0) {
        printf("Failed to close lock lock file\n");
        return -1;
    }
    return 0;
}

int closeLockLock(char *filename) {
    /*
     remove lock lock file, lock file could be modified by others
     */
    
    printf("Removing lock lock file\n");
    
    char lockFilename[MAX_LOCK_FILENAME_LENGTH + strlen(LOCK_FILE_EXT)];
    memset(lockFilename, '\0', MAX_LOCK_FILENAME_LENGTH);
    memcpy(lockFilename, filename, strlen(filename));
    strcat(lockFilename, LOCK_FILE_EXT);
    
    if (remove(lockFilename)) {
        printf("Failed to remove lock lock file\n");
        return -1;
    }
    printf("Finish removing lock lock file\n");
    return 0;
}

bool checkLock(char *filename, enum LOCK_TYPES lock, int offset, int limit) {
    /*
     check is there is lock for the file
     find conflict line in lock file, if could't, there is no lock
     */
    
    char lockFilename[MAX_LOCK_FILENAME_LENGTH];
    memset(lockFilename, '\0', MAX_LOCK_FILENAME_LENGTH);
    memcpy(lockFilename, filename, strlen(filename));
    strcat(lockFilename, LOCK_FILE_EXT);
    
    if (createLockLock(lockFilename, LOCK_READ) < 0) {
        printf("Failed to create lock lock file\n");
        if (closeLockLock(lockFilename) < 0) {
            printf("Failed to close lock lock file\n");
            return -1;
        }
        return -1;
    }
    
    FILE *lockFd = fopen(lockFilename, "r");
    if (!lockFd) {
        printf("Failed to open file %s: there is no lock\n", lockFilename);
        if (closeLockLock(lockFilename) < 0) {
            printf("Failed to close lock lock file\n");
            return -1;
        }
        return false;
    }
    
    int pid, locks_count, loffset, llimit;
    enum LOCK_TYPES currentLock;
    bool hasConflict = false;
    int locks_border = 0;
    while ((locks_count = fscanf(lockFd, LOCK_LINE_PATTERN, &pid, &currentLock, &loffset, &llimit)) >= 0) {
        locks_border++;
        if (locks_border >= MAX_NUMBER_OF_LOCKS) {
            if (closeLockLock(lockFilename) < 0) {
                printf("Failed to close lock lock file\n");
                return -1;
            }
            return true;
        }
        
        if ((conflictLockTypes(currentLock, lock)) && (hasIntersection(loffset, llimit, offset, limit))) {
            hasConflict = true;
            break;
        }
    }
    
    if (closeLockLock(lockFilename) < 0) {
        printf("Failed to close lock lock file\n");
        return -1;
    }
    
    return hasConflict;
}

int createLock(char *filename, enum LOCK_TYPES lock, int offset, int limit) {
    /*
     create lock file for the file, add lock line for the offset and limit
     */
    
    char lockFilename[MAX_LOCK_FILENAME_LENGTH];
    memset(lockFilename, '\0', MAX_LOCK_FILENAME_LENGTH);
    memcpy(lockFilename, filename, strlen(filename));
    strcat(lockFilename, LOCK_FILE_EXT);
    
    if (createLockLock(lockFilename, LOCK_WRITE) < 0) {
        printf("Failed to create lock lock file\n");
        return -1;
    }
    
    FILE *lockFd = fopen(lockFilename, "a+");
    if (!lockFd) {
        printf("Failed to open for writing lock file %s\n", lockFilename);
        if (closeLockLock(lockFilename) < 0) {
            printf("Failed to close lock lock file\n");
            return -1;
        }
        return -1;
    }
    
    fseek(lockFd, 0, SEEK_END);
    
    if ((fprintf(lockFd, LOCK_LINE_PATTERN, getpid(), lock, offset, limit)) < 0) {
        printf("Failed to write lock file %s\n", lockFilename);
        if (closeLockLock(lockFilename) < 0) {
            printf("Failed to close lock lock file\n");
            return -1;
        }
        return -1;
    }
    
    if (fclose(lockFd) < 0) {
        printf("Failed to close lock file %s\n", lockFilename);
        if (closeLockLock(lockFilename) < 0) {
            printf("Failed to close lock lock file\n");
            return -1;
        }
        return -1;
    }
    
    if (closeLockLock(lockFilename) < 0) {
        printf("Failed to close lock lock file\n");
        return -1;
    }
    
    return 0;
}

FILE* openFile(char *filename, enum LOCK_TYPES lock, int offset, int limit) {
    /*
     check if file is not locked, if not, open it
     */
    
    int tries_count = 0;
    while ((tries_count < MAX_NUMBER_OF_TRIES) && (checkLock(filename, lock, offset, limit))) {
        sleep(WAIT_TIMEOUT);
        tries_count++;
    }
    
    if (createLock(filename, lock, offset, limit) < 0) {
        printf("Failed to create lock for %s\n", filename);
        return NULL;
    }
    
    FILE *fd = (lock == LOCK_READ) ? fopen(filename, "r") : fopen(filename, "r+");
    return fd;
}

int closeFile(char *filename, enum LOCK_TYPES lock, int offset, int limit) {
    /*
     close file and remove lock
     */
    
    char lockFilename[MAX_LOCK_FILENAME_LENGTH];
    memset(lockFilename, '\0', MAX_LOCK_FILENAME_LENGTH);
    memcpy(lockFilename, filename, strlen(filename));
    strcat(lockFilename, LOCK_FILE_EXT);
    
    if (createLockLock(lockFilename, LOCK_WRITE) < 0) {
        printf("Failed to create lock lock file\n");
        return -1;
    }
    
    FILE *lockFd = fopen(lockFilename, "r");
    if (!lockFd) {
        printf("Failed to open lock file %s\n", lockFilename);
        if (closeLockLock(lockFilename) < 0) {
            printf("Failed to close lock lock file\n");
            return -1;
        }
        return -1;
    }
    
    struct Lock locks[MAX_NUMBER_OF_LOCKS];
    memset(&locks, 0, MAX_NUMBER_OF_LOCKS * sizeof(struct Lock));
    
    int lock_idx = 0;
    int pid, loffset, llimit;
    enum LOCK_TYPES lock_type;
    while (fscanf(lockFd, LOCK_LINE_PATTERN, &pid, &lock_type, &loffset, &llimit) >= 0) {
        if ((pid == getpid()) && (lock_type == lock) && (loffset == offset) && (llimit == limit)) {
            continue;
        }
        locks[lock_idx].pid = pid;
        locks[lock_idx].lock = lock_type;
        locks[lock_idx].offset = loffset;
        locks[lock_idx].limit = llimit;
        printf(LOCK_LINE_PATTERN, locks[lock_idx].pid, locks[lock_idx].lock, locks[lock_idx].offset, locks[lock_idx].limit);
        lock_idx++;
    }

    if (fclose(lockFd) < 0) {
        printf("Failed to close lock file %s\n", lockFilename);
        if (closeLockLock(lockFilename) < 0) {
            printf("Failed to close lock lock file\n");
            return -1;
        }
        return -1;
    }
    
    lockFd = fopen(lockFilename, "w");
    if (!lockFd) {
        printf("Failed to open lock file %s\n", lockFilename);
        if (closeLockLock(lockFilename) < 0) {
            printf("Failed to close lock lock file\n");
            return -1;
        }
        return -1;
    }
    
    int i;
    for (i=0; i<lock_idx; i++) {
        printf("Start re-write lock file\n");
        printf(LOCK_LINE_PATTERN, locks[i].pid, locks[i].lock, locks[i].offset, locks[i].limit);
        if (fprintf(lockFd, LOCK_LINE_PATTERN, locks[i].pid, locks[i].lock, locks[i].offset, locks[i].limit) < 0) {
            printf("Failed to write lock file %s\n", lockFilename);
            if (closeLockLock(lockFilename) < 0) {
                printf("Failed to close lock lock file\n");
                return -1;
            }
            return -1;
        }
    }
    
    if (fclose(lockFd) < 0) {
        printf("Failed to close lock file %s\n", lockFilename);
        if (closeLockLock(lockFilename) < 0) {
            printf("Failed to close lock lock file\n");
            return -1;
        }
        return -1;
    }
    
    if (closeLockLock(lockFilename) < 0) {
        printf("Failed to close lock lock file\n");
        return -1;
    }
    
    return 0;
}

size_t readFile(char *filename, int offset, int limit, char *buffer) {
    /*
     open file for reading with lock on limit part from offset if it's not locked for reading
     */
    
    printf("Try to read file %s: %d bytes from position %d", filename, offset, limit);
    
    FILE *fd = openFile(filename, LOCK_READ, offset, limit);
    if (!fd) {
        printf("Failed to open file %s for reading max number of times\n", filename);
        closeFile(filename, LOCK_READ, offset, limit);
        return -1;
    }
    
    sleep(5);
    if ((fseek(fd, offset, 0)) < 0) {
        printf("Failed to shift to offset %d in file %s\n", offset, filename);
        closeFile(filename, LOCK_READ, offset, limit);
        return -1;
    }
    
    size_t read_bytes = fread(buffer, sizeof(char), limit, fd);
    closeFile(filename, LOCK_READ, offset, limit);
    return read_bytes;
}

size_t writeFile(char *filename, int offset, int limit, char *buffer) {
    /*
     open file for writing with lock on limit part from offset if it's not locked for writing
     */
    
    printf("Try to write file %s: %d bytes from position %d\n", filename, offset, limit);
    
    FILE *fd = openFile(filename, LOCK_WRITE, offset, limit);
    if (!fd) {
        printf("Failed to open file %s for writing max number of times\n", filename);
        closeFile(filename, LOCK_WRITE, offset, limit);
        return -1;
    }
    
    sleep(5);
    
    if ((fseek(fd, offset, SEEK_SET)) < 0) {
        printf("Failed to shift to offset %d in file %s\n", offset, filename);
        closeFile(filename, LOCK_WRITE, offset, limit);
        return -1;
    }
    
    size_t write_bytes = fwrite(buffer, sizeof(char), limit, fd);
    closeFile(filename, LOCK_WRITE, offset, limit);
    return write_bytes;
}

int main(int argc, const char * argv[]) {
    if ((argc < 4) || (!strcmp(argv[1], "-h")) || (!strcmp(argv[1], "--help"))) {
        printf("Usage: ./lock_file mode offset limit\n");
        printf("mode = -r for test read or -w for test write\n");
        return 0;
    }
    
    int offset = atoi(argv[2]);
    int limit = atoi(argv[3]);
    
    char buffer[BUFFER_SIZE];
    if (!strcmp(argv[1], "-r")) {
        if ((readFile(TEST_FILENAME, offset, limit, buffer)) != limit) {
            printf("Failed to read file\n");
            return -1;
        }
    } else {
        if ((writeFile(TEST_FILENAME, offset, limit, argv[4])) != limit) {
            printf("Failed to write file\n");
            return -1;
        }
    }
    return 0;
}
