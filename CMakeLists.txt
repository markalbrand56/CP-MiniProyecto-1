cmake_minimum_required(VERSION 3.28)
project(MiniProyecto_1)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")

add_executable(MiniProyecto_1 main.cpp)
