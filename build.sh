#!/usr/bin/env sh

DEBUG=1
DO_BUILD=1

SETTINGS="-std=c99 -Wextra -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Wno-comment"
SOURCE_FILE="test.c";

if [ -z "$1" ]; then
    echo "Please pass the name of a library to test, such as \"prof\" or \"csv\".";
    exit 1
fi

if [ "$1" == "csv" ]; then
    TEST_NAME="csv";
elif [ "$1" == "prof" ]; then
    TEST_NAME="prof";
else
    echo "Cannot find library with the name \"$1\"";
    DO_BUILD=0;
fi

if [ $DO_BUILD -eq 1 ]; then
    BUILD_FLAGS="-DRYN_TEST_$TEST_NAME";

    if [ $DEBUG -eq 0 ]; then
        echo "Optimized build";
        TARGET="-O2 -o test_$TEST_NAME.exe"
    elif [ $DEBUG -eq 1 ]; then
        echo "Debug build";
        TARGET="-g3 -O0 -o test_$TEST_NAME.out"
    fi

    echo "$SETTINGS $BUILD_FLAGS $SOURCE_FILE $TARGET";
    gcc $SETTINGS $BUILD_FLAGS $SOURCE_FILE $TARGET;
fi
