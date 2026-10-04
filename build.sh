#!/usr/bin/env bash
goal=$1
if [ $goal = 'build' ]; then
    cmake -B build
    cmake --build build
elif [ $goal = 'run' ]; then
    echo $goal
    ./build/program
else
    echo "pass valid build parameter (run or build)"
fi