# Getting It Up and Running

This project is known to work on Ubuntu 22.04
(and its derivatives) and Windows 11. It might
still work on other platforms with some tweaks.

- [Getting It Up and Running](#getting-it-up-and-running)
  - [Setup](#setup)
    - [Build LightGBM](#build-lightgbm)
    - [Pick your Model](#pick-your-model)
    - [Compile](#compile)
  - [Run](#run)

## Setup

### Build LightGBM

Go to <https://github.com/microsoft/LightGBM/releases/tag/v4.6.0>
and select file `LightGBM-complete_source_code_zip.zip` for download.

Unzip it into some directory.

Please set the following environment variables:

- `$PROJECT_ROOT` is the root directory of this project
- `$LGBM_ROOT` is the root directory of the LightGBM project
    (that you've just downloaded). This should be set
    system-wide
- `$LGBM_MODEL_FILE` is the path to a trained LightGBM model file
    that you wish to use.

Then, build the (static) LightGBM C library.

- On Linux, Bash:

    ```sh
    cd $LGBM_ROOT
    mkdir build && cd build
    cmake .. -DBUILD_STATIC_LIB=ON -DCMAKE_BUILD_TYPE=Release
    make -j$(nproc)
    ```

- On Windows, PowerShell:

    ```powershell
    cd "$LGBM_ROOT"
    mkdir build
    cd build
    cmake .. -DBUILD_STATIC_LIB=ON -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release --target ALL_BUILD
    ```

### Pick your Model

A pretrained model from the EMBER2024
project, named `EMBER2024_all.model`, has
been added in the source code of this
project, under `$PROJECT_ROOT/model/resources`.

You could replace it with another
**LGBM Booster binary model**, but
keep the same file name (or you will
have to change the file name in
`$PROJECT_ROOT/model/src/resource.cpp`).

### Compile

- On Linux:

    ```sh
    cd $PROJECT_ROOT
    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target all -j
    ```

- On Windows (VS2022):

    ```powershell
    cd $PROJECT_ROOT
    mkdir build
    cd build
    cmake .. -A x64 -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release --target ALL_BUILD
    ```

## Run

- On Linux:

    ```sh
    cd $PROJECT_ROOT
    cd build
    # You may also need to
    #    cd demo
    # if you use custom build tools e.g. Ninja
    
    ./ember2024_lgbm_demo <path/to/PE/file/to/scan>
    ```

- On Windows (VS2022):

    ```powershell
    cd $PROJECT_ROOT
    cd build/Release
    .\ember2024_lgbm_demo.exe <path/to/PE/file/to/scan>
    ```
