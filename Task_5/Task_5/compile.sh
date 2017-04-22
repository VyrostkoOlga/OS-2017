#!/bin/sh

#  compile.sh
#  Task_5
#
#  Created by Ольга Выростко on 22.04.17.
#  Copyright © 2017 Ольга Выростко. All rights reserved.

gcc -std=c99 -D_POSIX_C_SOURCE=199309L fork_pid.c -lm -lrt -o fork_pid
