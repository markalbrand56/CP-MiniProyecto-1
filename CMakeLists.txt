cmake_minimum_required(VERSION 3.28)
project(MiniProyecto_1 C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")

add_executable(MiniProyecto_1 main.c)
