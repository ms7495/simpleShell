cmake_minimum_required(VERSION 3.27)
project(simpleShell C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(simpleShell
        main.c
        main.h)
