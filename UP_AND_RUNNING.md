# Getting It Up and Running

This project is known to work on Ubuntu 22.04
(and its derivatives) and Windows 10+. It might
still work on other POSIX platforms (e.g. MacOS)
with some tweaks.

**ATTENTION: If you stumble upon a problem,**
**besure to check the [Troubleshooting](#troubleshooting-building-on-windows)**
**section first. Plenty of gotcha's there!**

- [Getting It Up and Running](#getting-it-up-and-running)
  - [Setup](#setup)
    - [Prerequisites](#prerequisites)
    - [Build LightGBM](#build-lightgbm)
    - [Pick your Model](#pick-your-model)
    - [Compile](#compile)
  - [Run](#run)
  - [Troubleshooting](#troubleshooting)
    - [`FAILED: [code=2] ... C1041: cannot open program database ... same .PDB file, please use /FS`](#failed-code2--c1041-cannot-open-program-database--same-pdb-file-please-use-fs)
    - [`FAILED: [code=1] ... deleting depfile: No error`](#failed-code1--deleting-depfile-no-error)
    - [`error: remove(...): The process cannot access the file because it is being used by another process.`](#error-remove-the-process-cannot-access-the-file-because-it-is-being-used-by-another-process)

## Setup

### Prerequisites

- On Linux:
  - GCC (gcc, g++)
  - Clang
  - Cmake
  - Ninja 1.10+
  - sccache (optional, but recommended)

- On Windows:
  - VS2022
  - CMake
  - Ninja 1.13+
  - sccache (optional, but recommended)

### Build LightGBM

Go to <https://github.com/microsoft/LightGBM/releases/tag/v4.6.0>
and select file `LightGBM-complete_source_code_zip.zip` for download.

Unzip it into some directory.

Please set the following environment variables:

- `$PROJECT_ROOT` is the root directory of this project.
    (This variable will only be used in the commands below,
    so it's optional that you set this variable.)

- `$LGBM_ROOT` is the root directory of the LightGBM project
    (that you've just downloaded and unzipped). Since this
    variable is used in `CMakeLists.txt` files in this project,
    it should be set system-wide (so that IDEs and compilers
    could read it).
                                                                            
Then, build the (static) LightGBM C library.

- On Linux, Bash:

    ```sh
    cd $LGBM_ROOT
    mkdir build && cd build
    cmake .. -G "Ninja" -DBUILD_STATIC_LIB=ON -DCMAKE_BUILD_TYPE=Release
    make -j$(nproc)
    ```

- On Windows, Developer PowerShell for VS2022:

    ```powershell
    cd "$env:LGBM_ROOT"
    mkdir build
    cd build
    cmake .. -G "Ninja" -DBUILD_STATIC_LIB=ON -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release --target all -j
    ```

    If you encounter some error like "file X cannot be opened
    because it is currently opened by another process"

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

- On Linux, Bash:

    ```sh
    cd $PROJECT_ROOT
    mkdir build
    cd build
    cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target all -j
    ```

- On Windows, Developer PowerShell for VS2022:

    ```powershell
    cd $env:PROJECT_ROOT
    mkdir build
    cd build
    cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
    cmake --build . --config Release --target all -j
    ```

## Run

- On Linux:

    ```sh
    cd $PROJECT_ROOT
    cd build
    cd demo
    
    ./ember2024_lgbm_demo <path/to/PE/file/to/scan>
    ```

- On Windows:

    ```powershell
    cd $env:PROJECT_ROOT
    cd build
    cd demo
    .\ember2024_lgbm_demo.exe <path/to/PE/file/to/scan>
    ```

## Troubleshooting

### `FAILED: [code=2] ... C1041: cannot open program database ... same .PDB file, please use /FS`

It's because multiple instances of the compiler
trying to access some intermediate build files
(e.g. `.PDB`, `.o`, etc.). Those files could
also be locked by antivirus software.

This is classic with **MSVC + Ninja on Windows**
and I've searched for a proper solution to no
avail. However, a workaround does the trick.

It's utterly simple: just rerun your cmake build
command, e.g.

```powershell
cmake --build . --config Release --target all -j
```

For each rerun Ninja would discover translation
units that have not been compiled/linked and
continue the progress, so after multiple fails
you would get the program compile.

**Note that you should rerun only when you see**
**this particular error.** Beware for your own
mistakes though - you must fix them since reruns
don't do that for you, apparently.

If you don't have time to babysit this,
but could leave the machine running, consider
this instead:

```powershell
cmake --build . --config Release --target all -j 1
```

That `1` means only one compiler instance will
be running at a time. No parallelization.

- Therefore, the build process would be slow as hell.
- How it works: A file won't be simultaneously accessed
    by multiple instances of the compiler anymore,
    simply because there is now just one compiler
    instance running at a time.
- It still won't fix the problem if it is caused by
    antivirus locking files. In that case you have
    to turn back to the "retry" strategy above.

### `FAILED: [code=1] ... deleting depfile: No error`

Same cause, same fix as [this error](#failed-code2--c1041-cannot-open-program-database--same-pdb-file-please-use-fs).

### `error: remove(...): The process cannot access the file because it is being used by another process.`

Same cause, same fix as [this error](#failed-code2--c1041-cannot-open-program-database--same-pdb-file-please-use-fs).

It is just that now it comes from **MinGW on Windows**,
doesn't it?
