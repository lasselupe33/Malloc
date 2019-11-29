#!/bin/bash

# the folder on the remote machine in which you have "bits.c"
LOCAL="./"
SUBDIR="malloclab-handout"
FILE="mm.c"

scp $LOCAL/$FILE cos.itu.dk:~/$SUBDIR/$FILE

ssh cos.itu.dk "cd $SUBDIR; make; ./mdriver; exit"