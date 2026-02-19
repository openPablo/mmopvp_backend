cmake_minimum_required(VERSION 3.14)
project(pvpmmo_backend C CXX)

set(CMAKE_C_STANDARD 11)
find_package(ZLIB REQUIRED)
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
        gamelogic/character_state_machines.c
        gamelogic/character_state_machines.h
        gamelogic/projectile.c
        gamelogic/projectile.h
        gamelogic/selfCast.c
        gamelogic/selfCast.h
        gamelogic/area_of_effect.c
        gamelogic/area_of_effect.h
)
target_link_libraries(pvpmmo_backend PRIVATE LibDataChannel::LibDataChannel ZLIB::ZLIB m)