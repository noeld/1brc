cmake_minimum_required(VERSION 3.28)
project(1brc)

set(CMAKE_CXX_STANDARD 20)

add_executable(1brc main.cpp)

add_executable(create-sample
        create-sample.c)
target_link_libraries(create-sample PRIVATE m)