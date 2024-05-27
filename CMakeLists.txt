cmake_minimum_required(VERSION 3.28)
project(1brc)

set(CMAKE_CXX_STANDARD 20)

find_package(fmt CONFIG REQUIRED)
find_package(argparse CONFIG REQUIRED)

option(USE_SIMPLE_PARSE_FLOAT "Use the simple float parser which avoids copying." ON)

add_compile_options(-Werror -Wall -Wconversion)

add_executable(1brc main.cpp
        mmapped_file.h
        statistics.h
        simple_parse_float.cpp
        simple_parse_float.h)
target_link_libraries(1brc PRIVATE fmt::fmt argparse::argparse)
#target_compile_options(1brc PRIVATE "-mavx2" -O3)
if (USE_SIMPLE_PARSE_FLOAT)
        target_compile_definitions(1brc PRIVATE USE_SIMPLE_PARSE_FLOAT)
        message(STATUS "Using simple_parse_float")
endif()

add_executable(create-sample
        create-sample.c)
target_link_libraries(create-sample PRIVATE m)

include(FetchContent)
FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/onqtam/doctest.git
        GIT_TAG v2.4.11
)
FetchContent_MakeAvailable(doctest)

enable_testing()

add_executable(simple_float_convert_doctest
        simple_parse_float.cpp
        simple_parse_float.h
        simple_float_convert_doctest.cpp)
target_link_libraries(simple_float_convert_doctest PRIVATE doctest::doctest fmt::fmt)
add_test(NAME sfc_test COMMAND simple_float_convert_doctest)