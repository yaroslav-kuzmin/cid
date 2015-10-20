#!/bin/bash

FILE=version.h
VERSION_GIT=$(git log -1 --pretty=format:"%"h)
sed -e "s/^#define VERSION_GIT       0xfffffff/#define VERSION_GIT       0x$VERSION_GIT/" $FILE.in > $FILE
exit 0
