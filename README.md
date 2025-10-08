# ember2024-cpp

- [ember2024-cpp](#ember2024-cpp)
  - [Introduction](#introduction)
  - [Getting It Up and Running](#getting-it-up-and-running)
  - [Implementation Key Points](#implementation-key-points)
  - [Known Issues](#known-issues)
  - [Author and Licensing](#author-and-licensing)

## Introduction

This project consists of:

1. A **C++ implementation** of the **EMBER2024 feature extractor** for PE files, and
2. Integration with LightGBM to allow **loading an EMBER2024 pretrained LGBM model**
    and **running inference** directly on a PE file.

EMBER2024 feature extractor is [originally written in Python](https://github.com/futurecomputing4ai/ember2024).

## Getting It Up and Running

[See this guide](./UP_AND_RUNNING.md).

The code is known to compile and run fine
on Windows 10 and Ubuntu 22.04/Linux Mint 21.3.
Other operating systems e.g. MacOS might
need specific CMake/compiler tweaks to
get it right.

## Implementation Key Points

- Uses memory mapping instead of loading the whole PE file onto memory.
- Implements a reduction approach to iterate over blocks of a PE file
    to extract features, instead of random access on the mapped memory.
    This way, blocks should be read into memory sequentially and on-demand
    by the OS, saving memory consumption and avoid hangs (at least on
    Windows - see [Known issues](#known-issues)).
- Uses [RE2 for fast regex matching](https://github.com/google/re2)
    in `StringExtractor` feature type,
    which improved speed substantially.
- For other implementation details, see the `README.md` file
    in each subdirectory, if any.

## Known Issues

1. While most feature types are known to produce
    results that are consistent with those generated
    by the original Python code,
    some do not. I will fix this as time
    goes on, and probably write unit tests
    for robust checks.

2. On Linux, implementation of `mio` - a memory mapping
    library - showed poor performance even when the
    code in this project deliberately iterates over
    a PE file sequentially, block-by-block - resulting
    in the program eating up RAM and eventually terminated
    by the OS in case the PE file is too big.

    This issue is not present on Windows though - scanning
    a 1 GB file is done in a blink of an eye.

    I might have to rewrite custom memory mapping solutions,
    for each platform, to address this in the future.

## Author and Licensing

Copyright (c) 2025 Vu Tung Lam.

Licensed under the MIT license (which is
compatible with the Apache 2.0 imposed by
the original project).

See [LICENSE.txt](./LICENSE.txt) for details.
