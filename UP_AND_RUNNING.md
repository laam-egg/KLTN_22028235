# Getting It Up and Running

- [Getting It Up and Running](#getting-it-up-and-running)
  - [Build Options](#build-options)
  - [Windows](#windows)
    - [Windows: Setup](#windows-setup)
    - [Windows: Build the Libraries only](#windows-build-the-libraries-only)
    - [Windows: Build for Testing](#windows-build-for-testing)
  - [Ubuntu and derivatives](#ubuntu-and-derivatives)
    - [Ubuntu: Setup](#ubuntu-setup)
    - [Ubuntu: Build the Libraries only](#ubuntu-build-the-libraries-only)
    - [Ubuntu: Build for Testing](#ubuntu-build-for-testing)

## Build Options

1. **Build the Libraries only:** so that you just get
    the library files, static or dynamic, to be used
    in your program. No tests would be built.

2. **Build for Testing:** so that you could run the
    tests.

## Windows

### Windows: Setup

This guide assumes the compiler toolchain
is VS2022 + CMake.

CMake `>= 3.31` and `< 4` is preferred.

Open Developer Command Prompt for
VS2022. Type `powershell` to open
a PowerShell inside it.

- Or you could open the Developer Command
    Prompt, start VSCode from there,
    and use the PowerShell inside
    VSCode.
- Or, if you're using Visual Studio,
    just straight up open a terminal
    inside it.

Then, run this to check whether
OpenMP is installed and enabled (it
should be, by default):

```powershell
echo "int main(){}" > test.cpp ; cl /openmp test.cpp ; .\test.exe ; echo $LastExitCode ; rm test.*
```

Expected output:

    Microsoft (R)...
    ...
    0

Just make sure the last line contains the number `0`.

### Windows: Build the Libraries only

Compile:

```powershell
cd <project_root>

mkdir build
cd build
cmake .. -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --target ALL_BUILD
```

The library you would need to link against
is `build\core\Release\efe_core.lib`.

Run the demo program:

```powershell
cd <project_root>

.\build\demo\Release\efe_demo.exe /path/to/a/PE/file
```

### Windows: Build for Testing

Compile:

```powershell
cd <project_root>

$BUILD_TYPE = "Release"
# or
# $BUILD_TYPE = "Debug"

mkdir build
cd build
cmake .. -A x64 -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DBUILD_TESTING=ON
cmake --build . --config "$BUILD_TYPE" --target ALL_BUILD
```

Run tests:

```powershell
cd <project_root>

cd build
ctest --test-dir . --output-on-failure
```

## Ubuntu and derivatives

### Ubuntu: Setup

```sh
sudo apt update
sudo apt install -y libomp-dev libabsl-dev
```

where `libabsl-dev` is a prerequisite for RE2
(which this library uses). **UPDATE: `libabsl-dev`**
**might not be needed ; you may try omitting**
**it and see what happens in the next steps.**

Then, ensure that OpenMP is installed, with:

```sh
# Expected output: 0
echo 'int main(){}' > test.cpp && clang++ -fopenmp test.cpp -o test -lomp && rm -f test.cpp && rm -f test && echo $?
```

### Ubuntu: Build the Libraries only

Compile:

```sh
cd <project_root>

export GENERATOR=Ninja

mkdir build
cd build
cmake -G $GENERATOR .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target all -j
```

The library you would need to link against
is `build/core/libefe_core.a`.

Run the demo program:

```sh
cd <project_root>

./build/demo/efe_demo /path/to/a/PE/file
```

### Ubuntu: Build for Testing

Compile:

```sh
cd <project_root>

export BUILD_TYPE=Release
export GENERATOR=Ninja
# or
# export BUILD_TYPE=Debug

mkdir build
cd build
cmake .. -G $GENERATOR -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DBUILD_TESTING=ON
cmake --build . --target all -j
```

Run tests:

```sh
cd <project_root>

cd build
ctest --test-dir . --output-on-failure
```
