#!/bin/sh

#  compile.sh
#  Task_4
#
#  Created by Ольга Выростко on 22.04.17.
#  Copyright © 2017 Ольга Выростко. All rights reserved.

# For Linux compilation only
# Mac OS + xCode just: gcc main.c -o main

gcc -std=c99 -D_POSIX_C_SOURCE=199309L -pthread main.c -lm -lrt -o main
gcc -std=c99 client.c -o client
