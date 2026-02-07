cmake_minimum_required(VERSION 3.14)
project(pvpmmo_backend C CXX)

set(CMAKE_C_STANDARD 11)

include(FetchContent)
FetchContent_Declare(
        libdatachannel
        GIT_REPOSITORY https://github.com/paullouisageneau/libdatachannel.git
        GIT_TAG        v0.24.1
)
FetchContent_MakeAvailable(libdatachannel)

add_executable(pvpmmo_backend
        main.c
        networking.c
        uthash.h
)
target_link_libraries(pvpmmo_backend PRIVATE LibDataChannel::LibDataChannel m)