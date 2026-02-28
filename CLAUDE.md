# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Realm is a single-header C library implementing a hash table with time-to-live key-value pairs, designed for caching use cases. See README.md for the full API specification.

## Project Structure

- `realm.h` - Single-header library (header + implementation)
- `hello.c` - Minimal usage example
- `tst/unit.c` - Unit tests for all APIs
- `tst/app.c` - Session cache simulation demonstrating recurrent usage
- `tst/bench.c` - Complexity benchmark proving O(1) hash table vs O(n) linked list
- `Makefile` - Build configuration

## Build Commands

This is a single-header library. To use:

```c
#define REALM_C      // include implementation (in ONE .c file only)
#include "realm.h"
```

Build using Makefile:

```bash
make           # build and run hello example
make hello     # build and run hello example
make tests     # build and run all tests with valgrind
make clean     # remove all binaries
```

Or compile manually:

```bash
gcc -Wall -Wextra -o hello.exe hello.c
gcc -Wall -Wextra -I. -o tst/unit.exe tst/unit.c
gcc -Wall -Wextra -I. -o tst/app.exe tst/app.c
gcc -Wall -Wextra -I. -o tst/bench.exe tst/bench.c
```
