#!/usr/bin/env bash

set -e

goal="${1:-}"

# Detect platform to pick the right CMake generator
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" || "$OSTYPE" == "cygwin" ]]; then
    GENERATOR="MinGW Makefiles"
else
    GENERATOR="Unix Makefiles"
fi

if [ "$goal" = "build" ]; then
    cmake -B build -S . -G "$GENERATOR"
    cmake --build build

elif [ "$goal" = "test" ]; then
    ./build/program test

elif [ "$goal" = "run" ]; then
    ./build/program

elif [ "$goal" = "clean" ]; then
    rm -rf build

else
    echo "Usage: $0 {build|test|run|clean}"
    exit 1
fi
