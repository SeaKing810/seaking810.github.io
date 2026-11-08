#!/bin/sh
set -eu
cd "$(dirname "$0")"
mkdir -p build
python3 benchmark/make_baseline.py
CXX=${CXX:-clang++}
"$CXX" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror benchmark/benchmark.cpp -o build/benchmark
./build/benchmark
