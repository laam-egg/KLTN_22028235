# ember2024-fe-cpp

- [ember2024-fe-cpp](#ember2024-fe-cpp)
  - [Introduction](#introduction)
  - [Getting It Up and Running](#getting-it-up-and-running)
  - [Implementation Key Points](#implementation-key-points)
  - [Known Issues](#known-issues)
  - [Performance Benchmarking](#performance-benchmarking)
  - [Author and Licensing](#author-and-licensing)

## Introduction

This project consists of a **C++ implementation** of the
**EMBER2024 feature extractor** for PE files. It does not
contain the code to directly run an AI model - it is now
[in another project](https://github.com/pe-malware-detection/ember2024-lgbm).

EMBER2024 feature extractor is [originally written in Python](https://github.com/futurecomputing4ai/ember2024).

## Getting It Up and Running

[See this guide](./UP_AND_RUNNING.md).

The code is known to compile and run fine
on Windows 10, Windows 11, and
Ubuntu 22.04/Linux Mint 21.3.
Other operating systems e.g. MacOS might
need specific CMake/compiler tweaks to
get it right.

## Implementation Key Points

- Uses memory mapping instead of loading the whole PE file onto memory.

- Implements a reduction approach to iterate over blocks of a PE file
    to extract features, instead of random access on the mapped memory.
    This way, blocks should be read into memory sequentially and on-demand
    by the OS, saving memory consumption and avoid hangs.

- Uses [RE2 for fast regex matching](https://github.com/google/re2)
    in `StringExtractor` feature type,
    which improved speed substantially.

- For other implementation details, see the `README.md` file
    in each subdirectory, if any.

- Using GoogleTest, a set of unit tests are implemented
    to verify the correctness of each feature type.

    - The PE binaries used in the tests are
        supplied in the `tests/resources/efe_test_data` directory,
        and are encrypted with a simple XOR cipher
        to avoid being flagged by antivirus software
        (yes, there are some real malware samples there).
    
    - The features are computed in prior by the original
        Python implementation, and stored in the
        SQLite3 database `index.sqlite3`, located in
        the same directory.

- All feature types are tested to be quite consistent with
    the original Python implementation, except for
    some known issues listed below.

## Known Issues

The test cases for all feature types as well
as the tolerance levels are [defined here](./tests/src/core/all_features.test.cpp).
Notable known issues:

1. `SectionInfo` feature type might differ slightly
    in some features, probably due to differences in
    PE parsing between `LIEF` and `pefile` libraries.
    Nevertheless, the differences are minor.

2. `ImportsInfo` faces the same challenge, but the
    differences are even smaller.

3. `PEFormatWarnings` is just impossible to match
    exactly, due to the very specific parsing logic
    of `pefile` that is hard to replicate in C++.
    The current implementation leaves the whole
    feature type with zero values.

    The pretrained EMBER2024 LGBM models might
    be affected by this. A workaround is to
    retrain the models without this feature type,
    either by cutting off the features or setting
    them all to zero.

## Performance Benchmarking

[See this file.](./benchmarks/procbench/README.md)

## Author and Licensing

Copyright (c) 2025 Vu Tung Lam.

Licensed under the MIT license (which is
compatible with the Apache 2.0 imposed by
the original project).

See [LICENSE.txt](./LICENSE.txt) for details.
