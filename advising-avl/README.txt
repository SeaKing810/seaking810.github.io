CS 499 Milestone Three | Enhancement Two
Sean McKenna | November 15, 2026
ABCU Course Advising with an AVL Tree

PURPOSE
Load a course catalog, print courses in order, and look up prerequisites.
The enhancement replaces the original unbalanced BST with an AVL tree,
adds bounded file processing, and preserves all-or-nothing catalog replacement.

CONTENTS
original/   Unmodified CS 300 source and supplied CSV.
enhanced/   Complete enhanced C++17 source and headers.
tests/      Invariants, boundary cases, and real-process compatibility checks.
benchmark/  Measurement program and reproducible original-tree instrumentation.
evidence/   Actual test log, raw measurements, medians, environment, checksums.
docs/       Algorithm reasoning, tradeoffs, input contract, and review mapping.

RUN (macOS or Linux terminal, from this extracted AdvisingAVL folder)
Requires a C++17 compiler, such as Apple clang or GCC. No external C++ libraries.

mkdir -p build
clang++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror enhanced/ProjectTwo.cpp -o build/advising
./build/advising

Choose 1; enter: original/ABCU_Advising_Program_Input.csv
Choose 2 for the sorted course list.
Choose 3; enter CSCI400 to see its prerequisites.
Choose 9 to exit. Paths containing spaces may be pasted with matching quotes.
The working directory determines how relative paths are resolved.

VERIFY
Requires Python 3 for standard-library test and benchmark scripts.
sh test.sh

This builds original/enhanced applications with warnings treated as errors and
runs the C++ tests with AddressSanitizer and UndefinedBehaviorSanitizer. Then it
compares seven complete command-line scenarios against the original binary.
To select another compatible compiler: CXX=g++ sh test.sh

BENCHMARK
sh benchmark.sh > build/benchmark-new.csv

The generated BaselineTree.hpp keeps the original tree logic, adding only
comparison counters and a height observer. The original source remains intact.
The CSV contains all five trials; evidence/benchmark-medians.csv summarizes the
recorded run. See docs/ALGORITHM_AND_RESULTS.txt before interpreting timings.

The original course artifact was committed August 20, 2026:
https://github.com/SeaKing810/cs-300-data-structures-portfolio/commit/266305cb24836ebd38efb855990436a2adc25b1f
The preserved copy was selected for CS 499 in October 2026.
