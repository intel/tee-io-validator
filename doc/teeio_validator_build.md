# TEE-IO Device Validation Utility Build

## Setup build environment

```
# For Ubuntu
sudo apt install build-essential

# For CentOS
sudo yum groupinstall 'Development Tools'
```

## Build binaries

```
git submodule update --init --recursive
cd teeio-validator
mkdir build
cd build
cmake -DARCH=x64 -DTOOLCHAIN=GCC -DTARGET=Debug -DCRYPTO=mbedtls ..
make -j
```
Below binaries are generated in `build/bin` directory.
- teeio_validator
- lside
- setide
