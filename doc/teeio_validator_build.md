# TEE-IO Device Validation Utility Build

## Setup build environment

```
# For Ubuntu
sudo apt install build-essential

# For CentOS 8
sudo dnf groupinstall 'Development Tools'
sudo dnf install dnf-plugins-core
sudo dnf config-manager --set-enabled powertools
sudo dnf install glibc-devel glibc-static

# For CentOS 9
sudo dnf groupinstall 'Development Tools'
sudo dnf install dnf-plugins-core
sudo dnf config-manager --set-enabled crb
sudo dnf install glibc-devel glibc-static

```

## Build binaries

```
git clone --single-branch -b main https://github.com/intel/tee-io-validator.git
cd tee-io-validator
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

### Example CMake commands

Here provides more CMake commands to replace the command in [Build binaries](#build-binaries)

```
cmake -DARCH=x64 -DTOOLCHAIN=GCC -DTARGET=Debug -DCRYPTO=mbedtls ..
```

```
cmake -DARCH=x64 -DTOOLCHAIN=GCC -DTARGET=Release -DCRYPTO=mbedtls ..
```

```
cmake -DARCH=x64 -DTOOLCHAIN=GCC -DTARGET=Debug -DCRYPTO=openssl ..
```

```
cmake -DARCH=x64 -DTOOLCHAIN=GCC -DTARGET=Release -DCRYPTO=openssl ..
```

### DiceTcbInfo extension support

To support DiceTcbInfo extension, please use the following CMake command to replace the command in [Build binaries](#build-binaries)

```
cmake -DARCH=x64 -DTOOLCHAIN=GCC -DTARGET=Debug -DCRYPTO=mbedtls -DX509_IGNORE_CRITICAL=ON ..
```

```
cmake -DARCH=x64 -DTOOLCHAIN=GCC -DTARGET=Release -DCRYPTO=mbedtls -DX509_IGNORE_CRITICAL=ON ..
```

```
cmake -DARCH=x64 -DTOOLCHAIN=GCC -DTARGET=Debug -DCRYPTO=openssl -DX509_IGNORE_CRITICAL=ON ..
```

```
cmake -DARCH=x64 -DTOOLCHAIN=GCC -DTARGET=Release -DCRYPTO=openssl -DX509_IGNORE_CRITICAL=ON ..
```

