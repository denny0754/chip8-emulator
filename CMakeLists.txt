cmake_minimum_required(VERSION 3.0)
project(CHIP8Emulator)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 17)

include_directories(vendors/)

set(SOURCES main.cpp chip8.cpp)

add_executable(CHIP8Emulator ${SOURCES})
link_directories("libs/")

target_link_libraries(CHIP8Emulator "-lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lversion -luuid -lSDL2 -lSDL2main")