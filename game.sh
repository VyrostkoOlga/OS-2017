#!/bin/bash

nc 127.0.0.1 1234
if [ $? == 1 ]; then
    nc -l 1234
else
    nc 127.0.0.1 1234
fi
