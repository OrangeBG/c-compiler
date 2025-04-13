#!/bin/bash
# My example bash scriptecho 
echo "Running..."
gcc -E -P main.c -o main.i
gcc -S -O -fno-asynchronous-unwind-tables -fcf-protection=none main.c
gcc main.s -o main