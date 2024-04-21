cmake_minimum_required(VERSION 3.28)
project(1brc)

set(CMAKE_CXX_STANDARD 20)

find_package(fmt CONFIG REQUIRED)

add_executable(1brc main.cpp)
target_link_libraries(1brc PRIVATE fmt::fmt)

add_executable(create-sample
        create-sample.c)
target_link_libraries(create-sample PRIVATE m)
