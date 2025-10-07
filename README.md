# EMBER2024 LMDB

- [EMBER2024 LMDB](#ember2024-lmdb)
  - [Windows](#windows)
    - [Windows: Setup](#windows-setup)
    - [Windows: Compile](#windows-compile)
    - [Windows: Run](#windows-run)
  - [Ubuntu and derivatives](#ubuntu-and-derivatives)
    - [Ubuntu: Setup](#ubuntu-setup)
    - [Ubuntu: Compile](#ubuntu-compile)
    - [Ubuntu: Run](#ubuntu-run)

## Windows

### Windows: Setup

This guide uses VS2022. I do not know
how to do it with a different compiler
toolchain e.g. MinGW. Maybe ChatGPT?

So first, install VS2022 and CMake.
CMake `>= 3.31` and `< 4` is preferred.

Open Developer Command Prompt for
VS2022. Type `powershell` to open
a PowerShell inside it. (Or you
could open the Developer Command
Prompt, start VSCode from there,
and use the PowerShell inside
VSCode.)

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

Then, build the (static) LightGBM C library:

```powershell
cd <project_root>\..
git clone https://github.com/microsoft/LightGBM
cd LightGBM
git checkout 047e7d5a2a227273608efa142bd3a7ddbd71ff5c
git submodule update --init --recursive

mkdir build
cd build
cmake .. -DBUILD_STATIC_LIB=ON -A x64
cmake --build . --config Release --target ALL_BUILD
```

### Windows: Compile

```powershell
cd <project_root>
mkdir build
cd build
cmake .. -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --target ALL_BUILD
```

### Windows: Run

```powershell
cd <project_root>/build/Release
.\my_program.exe <path/to/lgbm/model/file>
```

## Ubuntu and derivatives

### Ubuntu: Setup

```sh
sudo apt update
sudo apt install -y libomp-dev libabsl-dev
```

where `libabsl-dev` is a prerequisite for RE2
(see below).

Then, check that OpenMP is installed, with:

```sh
# Expected output: 0
echo 'int main(){}' > test.cpp && clang++ -fopenmp test.cpp -o test -lomp && rm -f test.cpp && rm -f test && echo $?
```

Meanwhile, build the (static) LightGBM C library:

```sh
cd <project_root>/..
git clone https://github.com/microsoft/LightGBM
cd LightGBM
git checkout 047e7d5a2a227273608efa142bd3a7ddbd71ff5c
git submodule update --init --recursive

mkdir build && cd build
cmake .. -DBUILD_STATIC_LIB=ON -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

and build the RE2 library:

```sh
cd <project_root>/..
git clone https://github.com/google/re2.git
cd re2
git checkout 61c4644171ee6b480540bf9e569cba06d9090b4b
git submodule update --init --recursive

rm -rf build
cmake -DRE2_TEST=OFF -DRE2_BENCHMARK=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -S . -B build
cd build
make
```

### Ubuntu: Compile

```sh
cd <project_root>
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build build --target all -j
```

### Ubuntu: Run

```sh
cd <project_root>/build
./my_program <path/to/lgbm/model/file>
```
