
#!/usr/bin/env bash

set -e
goal="${1:-}"
if [ "$goal" = "build" ]; then
    cmake -B build
    cmake --build build

elif [ "$goal" = "test" ]; then
    ./build/program test

elif [ "$goal" = "run" ]; then
    ./build/program

else
    echo "Usage: $0 {build|test|run}"
    exit 1
fi
