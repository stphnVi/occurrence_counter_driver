cmake_minimum_required(VERSION 3.30)
project(Encryption C)

set(CMAKE_C_STANDARD 11)

add_executable(Encryption main.c
        fieldArithmetic.c
        fieldArithmetic.h)
