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
