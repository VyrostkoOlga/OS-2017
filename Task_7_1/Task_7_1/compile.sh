#!/bin/sh

#  compile.sh
#  Task_7_1
#
#  Created by Ольга Выростко on 24.04.17.
#  Copyright © 2017 Ольга Выростко. All rights reserved.

gcc ./launcher.c -o launcher
gcc ./data_source.c -o data_source
gcc ./data_handler.c -o data_handler
gcc ./data_dest.c -o data_dest
