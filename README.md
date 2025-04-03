## Bptree

[![Tests](https://img.shields.io/github/actions/workflow/status/habedi/bptree/tests.yml?label=tests&style=flat&labelColor=282c34&logo=github)](https://github.com/habedi/bptree/actions/workflows/tests.yml)
[![Lints](https://img.shields.io/github/actions/workflow/status/habedi/bptree/lints.yml?label=lints&style=flat&labelColor=282c34&logo=github)](https://github.com/habedi/bptree/actions/workflows/lints.yml)
[![Benches](https://img.shields.io/github/actions/workflow/status/habedi/bptree/benches.yml?label=benches&style=flat&labelColor=282c34&logo=github)](https://github.com/habedi/bptree/actions/workflows/benches.yml)
[![Code Coverage](https://img.shields.io/codecov/c/github/habedi/bptree?label=coverage&style=flat&labelColor=282c34&logo=codecov)](https://codecov.io/gh/habedi/bptree)
[![CodeFactor](https://img.shields.io/codefactor/grade/github/habedi/bptree?label=code%20quality&style=flat&labelColor=282c34&logo=codefactor)](https://www.codefactor.io/repository/github/habedi/bptree)
[![License](https://img.shields.io/badge/license-MIT-007ec6?label=license&style=flat&labelColor=282c34&logo=open-source-initiative)](https://github.com/habedi/bptree/blob/main/LICENSE)
[![Release](https://img.shields.io/github/release/habedi/bptree.svg?label=release&style=flat&labelColor=282c34&logo=github)](https://github.com/habedi/bptree/releases/latest)

Bptree is a B+tree implementation in pure C.

### Features

- Single-file C header implementation (see [include/bptree.h](include/bptree.h))
- Supports C99 or newer
- Generic key-value support and tunable node capacity
- Custom memory allocation support (via user-provided `malloc` and `free`)
- Supports insertion, deletion, search, and range queries

### Getting Started

To use Bptree, download the [include/bptree.h](include/bptree.h) file
and include it in your project like this:

```c
// Add these lines to the top of your C or C++ source files
#define BPTREE_IMPLEMENTATION
#include "bptree.h"
```

### Examples

Check out the [test/test_bptree.c](test/test_bptree.c) file for usage examples and test cases,  
and the [test/bench_bptree.c](test/bench_bptree.c) file for performance benchmarks.

Tests and benchmarks can be run using the provided [Makefile](Makefile).

```bash
# Run the tests
make test

# Run the benchmarks
make bench
```

### Portability

Bptree is tested on Ubuntu 24.04 LTS (AMD64) with GCC 13.3 and Clang 18.1.  
It should also work on most Unix-like environments (for example, other GNU/Linux distros, macOS, and BSDs)
and Windows (via MinGW or Cygwin).

### Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for details on how to make a contribution.

### License

Bptree is licensed under the MIT License ([LICENSE](LICENSE)).
