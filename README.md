

Install libdatachannel:

```shell
git clone --recursive https://github.com/paullouisageneau/libdatachannel.git
cd libdatachannel
git submodule update --init --recursive
cmake -B build -DUSE_GNUTLS=0 -DUSE_NICE=0 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
sudo cmake --install build
```