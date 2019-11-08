#!/bin/bash

# the folder on the remote machine in which you have "bits.c"
LOCAL="./"
SUBDIR="malloclab-handout"
FILE="mm.h"
LIB="memlib.h"

scp cos.itu.dk:~/$SUBDIR/$FILE $LOCAL/$FILE
scp cos.itu.dk:~/$SUBDIR/$LIB $LOCAL/$LIB

