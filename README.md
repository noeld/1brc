# 1 Billion Row Challenge


- Challenge blog post: https://www.morling.dev/blog/one-billion-row-challenge/
- Challenge repository: https://github.com/gunnarmorling/1brc
- Challenge C/C++ repository: https://github.com/dannyvankooten/1brc

This is my take on the [1 Billion Row Challenge](https://1brc.dev/) in C++. For
personal interests, I did not quite follow all the
[rules](https://1brc.dev/#rules-and-limits) for this challenge strictly.
Specifically:

- I used two external libraries (see [vcpkg.json](vcpkg.json)):
    - [fmt](https://github.com/fmtlib/fmt) for nicely formatted output
    - [argparse](https://github.com/p-ranav/argparse) for command line argument
    parsing
- I wanted at least some sound handling of unexpected conditions, e.g. I spend
    a little effort on error messages, format checking etc. Though it is
    interesting of course how far you can push the CPU using all available
    information on the structure of the data, I think I would advice against it
    in any production environment.
- Code is split up into several files.

## Specifics

The code uses `mmap` to map the CSV file into memory. On my computer with 16 GB
of RAM mapping the whole file into memory cluttered all the memory and the
computer startet swapping. Therefore the memory is mapped and unmapped in
chunks of 64MB size. Using `MAP_SHARED` in place of `MAP_PRIVATE` seems to improve
this.

Threads are used to parallelize workload. The file is simply split into
partitions of equal size. Each thread handles one partition. At the start of
each partition (except for the first) the thread scans for the first newline
character at (start of partition) - 1 offset. At the end of each partition each
thread finishes the currently processed line, fetching an additional mmapped
chunk if necessary. (In case the last line crosses partition boundaries.)

A custom function `simple_parse_float` is used since I found no way to parse
float values without copying the memory in the standard library.

## Build

I have only run this on GNU/Linux.

Apart from a C++ compiler you need `cmake` and `vcpkg`. Also, I used `ninja` as
generator tool.

Download or clone the files into a folder, switch to that folder. Then:

    mkdir build
    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    cmake --build build

If you want to use `std::stof` instead of `simple_parse_float` you can turn the
compile option off like `cmake ... -DUSE_SIMPLE_PARSE_FLOAT=OFF ...`.

## Run

1. Create test data using `build/create-sample 1000000000`.
1. Run the challenge using `time build/1brc measurements.txt > /dev/null`.


## Usage

    Usage: 1brc [--help] [--version] [--threads THREADS] [--verbose] file

    Positional arguments:
      file                   input CSV file with two columns: STATION;DEGREES [required]

    Optional arguments:
      -h, --help             shows help message and exits
      -v, --version          prints version information and exits
      -T, --threads THREADS  Use specified number of threads
      -V, --verbose          print verbose output

## Measured Results

Using *hot* cache 5 consecutive executions yielded a mean of 7,64s to parse a
measurements.txt file with 1 billion rows ~ 12,8GB.

    ./1brc measurements.1E9.txt > /dev/null  75,04s user 1,83s system  986% cpu 7,788 total
    ./1brc measurements.1E9.txt > /dev/null  82,36s user 1,59s system 1049% cpu 7,999 total
    ./1brc measurements.1E9.txt > /dev/null  74,16s user 2,25s system  975% cpu 7,831 total
    ./1brc measurements.1E9.txt > /dev/null  79,48s user 1,54s system 1055% cpu 7,673 total
    ./1brc measurements.1E9.txt > /dev/null  74,62s user 1,74s system 1105% cpu 6,909 total

on a AMD Ryzen 5 3600 6-Core Processor.

