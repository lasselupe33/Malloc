#!/bin/bash

# the folder on the remote machine in which you have "bits.c"
LOCAL="./"
SUBDIR="malloclab-handout"
FILE="mm.h"
LIB="memlib.h"

scp cos.itu.dk:/opt/traces/ $LOCAL/traces

