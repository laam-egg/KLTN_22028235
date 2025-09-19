# EMBER2024 LMDB

## Windows

**TODO**

## Ubuntu and derivatives

### Setup

```sh
sudo apt update
sudo apt install -y libomp-dev
```

then check that OpenMP is installed, with:

```sh
# Expected output: 0
echo 'int main(){}' > test.cpp && clang++ -fopenmp test.cpp -o test -lomp && rm test.cpp && echo $?
```

Meanwhile, build the (static) LightGBM C library:

```sh
cd <project_root>/..
git clone --recursive https://github.com/microsoft/LightGBM
cd LightGBM

mkdir build && cd build
cmake -DBUILD_STATIC_LIB=ON ..
make -j$(nproc)
```

### Compile

```sh
cd <project_root>
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build build --target all -j
```

### Run

```sh
cd <project_root>/build
./my_program <path/to/lgbm/model/file>
```
