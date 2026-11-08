#!/bin/sh
set -eu
cd "$(dirname "$0")"
mkdir -p build
CXX=${CXX:-clang++}
"$CXX" -std=c++17 -Wall -Wextra -Wpedantic -Werror -O2 enhanced/ProjectTwo.cpp -o build/advising
"$CXX" -std=c++17 -Wall -Wextra -Wpedantic -Werror -O2 original/ProjectTwo.cpp -o build/original
"$CXX" -std=c++17 -Wall -Wextra -Wpedantic -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer tests/test_advising.cpp -o build/tests
./build/tests
python3 tests/test_cli.py
