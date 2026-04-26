# Getting It Up and Running

This project is known to work on Ubuntu 22.04
(and its derivatives) and Windows 10+. It might
still work on other POSIX platforms (e.g. MacOS)
with some tweaks.

**ATTENTION: If you stumble upon a problem,**
**be sure to check the [Troubleshooting](#troubleshooting-building-on-windows)**
**section first. Plenty of gotcha's there!**

- [Getting It Up and Running](#getting-it-up-and-running)
  - [Setup](#setup)
    - [Prerequisites](#prerequisites)
    - [Build LightGBM](#build-lightgbm)
    - [Setup for Avast Authenticode](#setup-for-avast-authenticode)
    - [Pick your Model](#pick-your-model)
    - [Compile](#compile)
  - [Run](#run)
  - [Replacing the Model](#replacing-the-model)
  - [Troubleshooting](#troubleshooting)
    - [`FAILED: [code=2] ... C1041: cannot open program database ... same .PDB file, please use /FS`](#failed-code2--c1041-cannot-open-program-database--same-pdb-file-please-use-fs)
    - [`FAILED: [code=1] ... deleting depfile: No error`](#failed-code1--deleting-depfile-no-error)
    - [`error: remove(...): The process cannot access the file because it is being used by another process.`](#error-remove-the-process-cannot-access-the-file-because-it-is-being-used-by-another-process)
    - [`cl.exe not found` or `CMAKE_C_COMPILER not set` in a VSCode terminal](#clexe-not-found-or-cmake_c_compiler-not-set-in-a-vscode-terminal)
    - [`FetchContent_MakeAvailable ... Configuring incomplete, errors occurred!`](#fetchcontent_makeavailable--configuring-incomplete-errors-occurred)

## Setup

### Prerequisites

- On Linux:
  - GNU compiler toolchain (gcc, g++)
  - Clang
  - CMake
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
    **it should be set system-wide** (so that CMake and the
    compiler toolchain could read it).
                                                                            
Then, build the (static) LightGBM C library.

- On Linux, Bash:

    ```sh
    cd $LGBM_ROOT
    mkdir build && cd build
    cmake .. -G "Ninja" -DBUILD_STATIC_LIB=ON -DCMAKE_BUILD_TYPE=Release
    cmake --build . --target all -j
    ```

- On Windows: From Start Menu, search for
    **x64 Native Tools Command Prompt for VS2022**,
    run it, then run `powershell` inside it.
    We shall call this
    **x64 Native Tools PowerShell for VS2022**.
    We will be using it for compilation on Windows for
    the rest of this guide.

    Okay, now type in:

    ```powershell
    cd "$env:LGBM_ROOT"
    mkdir build
    cd build
    cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release `
        -DBUILD_STATIC_LIB=ON `
        -DBUILD_SHARED_LIBS=OFF `
        -DCMAKE_CXX_FLAGS="/MT /EHsc" `
        -DCMAKE_CXX_FLAGS_RELEASE="/MT /EHsc /O2 /DNDEBUG" `
        -DCMAKE_C_FLAGS="/MT" `
        -DCMAKE_C_FLAGS_RELEASE="/MT /O2 /DNDEBUG" `
        -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded" `
        -D__BUILD_FOR_R=OFF `
        -DUSE_OPENMP=ON `
        -DCMAKE_POLICY_VERSION_MINIMUM="3.5"
    
    cmake --build . --config Release --target all -j
    ```

    This CMake flag hell is due to MSVC libraries
    clashing each other if they have different settings
    on linking against the C Runtime, Debug/Release
    configurations, among other things. Yes, I hate
    that too.

    Why would we want to link against static libraries?
    This way the compiled binaries no longer need
    VC Redistributables! Within the context of
    developing antivirus software, I think that is
    an extremely valuable trait - less dynamic
    dependencies, less attack surface.

### Setup for Avast Authenticode

Install OpenSSL.

On Windows, approaches include:

- Using chocolatey: `choco install openssl`
- Using Shining Light's Win64 OpenSSL installer: <https://slproweb.com/products/Win32OpenSSL.html>

On Ubuntu/Debian:

```sh
sudo apt-get install openssl libssl-dev 
```

[Reference](https://github.com/avast/authenticode-parser?tab=readme-ov-file#build-installation-and-testing)

### Pick your Model

A pretrained model from the EMBER2024
project, named `EMBER2024_all.txt`, has
been added in the source code of this
project, under `$PROJECT_ROOT/model/resources`.

You could replace it with another
**LGBM Booster binary model**, but
keep the same file name (or you will
have to change the file name in
`$PROJECT_ROOT/model/src/resource.cpp`).

[The model files are uploaded onto here](https://huggingface.co/joyce8/EMBER2024-benchmark-models/tree/main)
by the authors of the EMBER2024 project.
Note that once downloaded, rename them
so that they have `.txt` extensions.
This is because LightGBM on Windows
will refuse to load models with newlines
that are not CRLF. When we let the model
files be text files, newlines should be
converted seamlessly by Git on Windows.
If not, run this on Windows:

```powershell
git config --global core.autocrlf true
```

On Linux, this is a non-issue.

### Compile

- On Linux, Bash:

    ```sh
    cd $PROJECT_ROOT
    mkdir build
    cd build
    cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
    cmake --build . --target all -j
    ```

- On Windows, again, in
    **x64 Native Tools PowerShell for VS2022**:

    ```powershell
    cd $env:PROJECT_ROOT
    mkdir build
    cd build
    cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DCMAKE_POLICY_VERSION_MINIMUM="3.5"
    cmake --build . --config Release --target all -j
    ```

    Again, we want to link against static libraries,
    including the C runtime. I've got you covered
    for that though, through holy hacks written in
    the `CMakeLists.txt` files, so you don't
    see nasty flags here anymore.

    While it is compiling, you could see some
    logs like

    ```
    cl : Command line warning D9025 : overriding '/MT' with '/MD'
    cl : Command line warning D9025 : overriding '/MD' with '/MT'
    ```

    Yes, those are our *hacks* at work, trying
    to get it compile and link against all libraries
    *statically*. Don't worry about that though.

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

## Replacing the Model

Note that the model file (which we selected
[here](#pick-your-model)) will be
embedded straight into the finally
compiled binaries, just like
Windows resources. **So if you change**
**the model, be sure to recompile.**

If you have compiled the whole project
before replacing the model, `ninja` would
not recompile since it sees no source
code change. In this case you
have to manually clean the
`ember2024_lgbm_model` target,
so that it appears to have not
been compiled, and `ninja` is
forced to recompile it, among
anything that depends on it.

- On Linux, Bash:
    
    ```sh
    cd build
    ninja -t clean ember2024_lgbm_model
    cmake --build . --target all -j
    ```

- On Windows, **x64 Native Tools PowerShell for VS2022**:
    
    ```powershell
    cd build
    ninja -t clean ember2024_lgbm_model
    cmake --build . --config Release --target all -j
    ```

## Troubleshooting

### `FAILED: [code=2] ... C1041: cannot open program database ... same .PDB file, please use /FS`

It's because multiple instances of the compiler
were trying to access some intermediate build files
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
**this particular error.** Beware for *your own*
*mistakes* though - you must fix them since reruns
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
    to turn back to the "retry" strategy above, or
    temporarily disable your antivirus' real-time
    protection or something.

### `FAILED: [code=1] ... deleting depfile: No error`

Same cause, same fix as [this error](#failed-code2--c1041-cannot-open-program-database--same-pdb-file-please-use-fs).

### `error: remove(...): The process cannot access the file because it is being used by another process.`

Same cause, same fix as [this error](#failed-code2--c1041-cannot-open-program-database--same-pdb-file-please-use-fs).

It is just that now it comes from **MinGW on Windows**,
doesn't it?

### `cl.exe not found` or `CMAKE_C_COMPILER not set` in a VSCode terminal

The latest version of VSCode (1.107) seems to not
inherit environment variables set
by the **x64 Native Tools Command Prompt for VS2022**
if there is another window of VSCode
already running.

To fix this, simply close all VSCode windows,
then launch VSCode from within that
MSVC command prompt again, and finally retry
intended cl/CMake commands.

### `FetchContent_MakeAvailable ... Configuring incomplete, errors occurred!`

This is likely due to CMake not being
able to access the internet or just
some temporary network glitch.

Run your intended CMake command again.
