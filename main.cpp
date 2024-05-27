// https://1brc.dev/#the-challenge
#include <atomic>
#include <clocale>
#include <exception>
#include <future>
#include <stdexcept>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <map>
#include <thread>
#include <unordered_map>
#include <vector>
//#include <pstl/glue_numeric_defs.h>
#include <fmt/core.h>
#include <argparse/argparse.hpp>

#include "mmapped_file.h"
#include "statistics.h"
#include "simple_parse_float.h"

/** Input File; UTF-8, UNIX line breaks 0x0a
00000000  4b 61 6e 73 61 73 20 43  69 74 79 3b 2d 30 2e 38  |Kansas City;-0.8|
00000010  0a 44 61 6d 61 73 63 75  73 3b 31 39 2e 38 0a 4b  |.Damascus;19.8.K|
00000020  61 6e 73 61 73 20 43 69  74 79 3b 32 38 2e 30 0a  |ansas City;28.0.|
00000030  4c 61 20 43 65 69 62 61  3b 31 37 2e 30 0a 44 61  |La Ceiba;17.0.Da|
00000040  72 77 69 6e 3b 32 39 2e  31 0a 4e 65 77 20 59 6f  |rwin;29.1.New Yo|
00000050  72 6b 20 43 69 74 79 3b  32 2e 31 0a 4c 69 73 62  |rk City;2.1.Lisb|
...
*/


struct simple_hasher {
    size_t operator()(void const *ptr, size_t len) {
        auto seed = static_cast<size_t>(0xc70f6907UL);
        auto const c = static_cast<unsigned char const *>(ptr);
        for (size_t i = 0; i < len; ++i)
            seed = 31 * seed + c[i];
        return seed;
    }
    size_t operator()(std::string_view const & sv) {
        return this->operator()(sv.data(), sv.length());
    }
};

struct string_hash {
    //using hash_type = std::hash<std::string_view>;
    using hash_type = simple_hasher;
    using is_transparent = void;

    std::size_t operator()(const char *str) const { return hash_type{}(str); }
    std::size_t operator()(std::string_view str) const { return hash_type{}(str); }
    std::size_t operator()(std::string const &str) const { return hash_type{}(str); }
};

using agg_map_type = std::unordered_map<std::string, statistics, string_hash, std::equal_to<> >;

/**
 * scan a part of input
 * .    .    .    .    .    .    .    .    .    .    .    .    .
 * Kansas;12.3\München;2.1\Hamburg;13.4\Blabla;34.4\Kairo;17.4\
 * *########################
 *                     ####*####################
 *                                    ##*######################
 * @param input input file
 * @param start offset in file from where to start; actually start _after_ the first new-line behind start, except if start == 0
 * @param end pffset in file where to stop; actually continue until the first new-line behind end
 * @return the map off aggregated values
 */
auto scan_input(mmapped_file const & input, size_t start, size_t end, size_t partition, bool verbose) -> agg_map_type {
    using std::string_view_literals::operator ""sv;
    agg_map_type map(1000);
    static constexpr size_t MAX_FIELDS_CNT = 10;
    std::vector<size_t> field_pos(MAX_FIELDS_CNT);
    size_t file_pos = start;
    size_t skipped = 0;
    while (file_pos < end) {
        if (start > 0 && file_pos == start)
            file_pos--;;
        auto chunk = input.get_chunk_for_offset(file_pos);
        auto sv = chunk.string_view().substr(chunk.initial_offset_);
        size_t i = 0;
        if (start > 0 && file_pos == start - 1) {
            // search for first new-line
            auto nl = sv.find(u8'\n');
            if (nl != std::string_view::npos) {
                skipped = nl + 1;
                i += skipped;
                if (verbose)
                    fmt::println(stderr, "Partition {:02} skipped {} bytes.", partition, i);
            } else {
                std::cerr << "Cannot find start of chunk" << (chunk.chunk_start_ + chunk.initial_offset_) << '\n';
                throw std::runtime_error("Cannot find start of chunk");
            }
        }
        field_pos.clear();
        field_pos.push_back(i);
        for (; i < sv.size(); ++i) {
            if (sv[i] != u8';' && sv[i] != u8'\n')
                continue;
            field_pos.push_back(i + 1);
            if (sv[i] == u8';') {
                if (field_pos.size() >= MAX_FIELDS_CNT) {
                    fmt::println(stderr, "Broken format in input file: too many fields at offset {}", (chunk.chunk_start_ + chunk.initial_offset_ + i));
                    throw std::runtime_error("Broken format in input file: too many fields");
                }
            }
            if (sv[i] == u8'\n') {
                if (field_pos.size() != 3) {
                    fmt::println(stderr, "Broken format in input file: too many fields at offset {}", (chunk.chunk_start_ + chunk.initial_offset_ + i));
                    throw std::runtime_error("Broken format in input file: not 2 fields");
                }
                auto station_view = sv.substr(field_pos[0], field_pos[1] - 1 - field_pos[0]);
                auto value_view = sv.substr(field_pos[1], field_pos[2] - 1 - field_pos[1]);
#ifdef USE_SIMPLE_PARSE_FLOAT
                float float_value;
                int parse_result = simple_parse_float(value_view, &float_value);
                if (parse_result > 0) {
                    fmt::println(stderr, "Broken format in input file: cannot parse float value {} at offset {}", value_view, (chunk.chunk_start_ + chunk.initial_offset_ + i));
                    throw std::runtime_error("Broken float value in input file.");
                }
#else
                float float_value = std::stof(std::string(value_view));
#endif
                auto found = map.find(station_view);
                if (found == map.end()) {
                    map.emplace(station_view, float_value);
                } else {
                    found->second.add_value(float_value);
                }
                ++i;
                field_pos.clear();
                field_pos.push_back(i);
                file_pos = chunk.chunk_start_ + chunk.initial_offset_ + i;
                if (file_pos >= end)
                    break;
            }
        }

    }
    if (verbose)
        fmt::println(stderr, "Partition {:02d} processed from {:12L} to actually {:12L} (end: {:12L})",
        partition, start + skipped,file_pos, end);
    return map;
}

// Benutzerdefinierter Vergleichsoperator für UTF-8-codierte Strings
struct UTF8StringComparator {
    bool operator()(const std::string& str1, const std::string& str2) const {
        // Verwende locale, um die UTF-8-codierten Strings zu vergleichen
        std::locale loc("");

        // Verwende std::use_facet, um das std::collate-Facet zu erhalten
        const std::collate<char>& coll = std::use_facet<std::collate<char>>(loc);

        // Vergleiche die Strings mit coll.compare
        return coll.compare(str1.data(), str1.data() + str1.length(),
            str2.data(), str2.data() + str2.length()) < 0;
    }
};

static constexpr int ERROR_ARGS = 1;
static constexpr int ERROR_FILE_FORMAT = 2;
static constexpr int ERROR_OTHER = 3;

int main(int argc, char *argv[]) {
    int ret = 0;
    argparse::ArgumentParser args("1brc", "1.0");
    args.add_argument("-T", "--threads").metavar(("THREADS")).help("Use specified number of threads").scan<'i', size_t>();
    args.add_argument("file").help("input CSV file with two columns: STATION;DEGREES").required();
    args.add_argument("-V", "--verbose").help("print verbose output").default_value(false).implicit_value(true);
    try {
        args.parse_args(argc, argv);
    } catch(std::exception const & e) {
        fmt::println(stderr, "{}", e.what());
        std::cerr << args;
        exit(ERROR_ARGS);
    }
    std::string file_name = args.get("file");
    bool const verbose = args.get<bool>("-V");
    mmapped_file input(file_name);
    if (input) {

        agg_map_type aggregated_result(1000);
        std::vector<std::jthread> threads;
        std::vector<std::future<void>> futures;
        std::mutex mtx;

        size_t max_chunks = (size_t)std::ceil(static_cast<double>(input.file_size()) / (double)input.chunk_size());
        size_t max_threads = args.present<size_t>("-T").value_or(std::thread::hardware_concurrency());
        auto partitions = std::min(max_chunks, max_threads);
        auto partition_size = (double)input.file_size() / (double)partitions;
        if (verbose){
            fmt::println(stderr, "Using chunk size of {}.", input.chunk_size());
            fmt::println(stderr, "File has size {}.", input.file_size());
            fmt::println(stderr, "Maximum of {} chunks.", max_chunks);
            fmt::println(stderr, "Using {} partitions (threads).", partitions);
        }

        for(size_t partition_nr = 0; partition_nr < partitions; ++partition_nr) {
            size_t start = (size_t)((double)partition_nr * partition_size);
            size_t end = (size_t)(((double)partition_nr + 1) * partition_size);
            std::promise<void> prm;
            futures.push_back(prm.get_future());
            threads.emplace_back([&input, &aggregated_result, &mtx, promise=std::move(prm), partition_nr, start, end, verbose] () mutable {
                if (verbose)
                    fmt::println(stderr, "Partition {:02} from {:9L} to {:9L}", partition_nr, start, end);
                try {
                    auto local_result = scan_input(input, start, end, partition_nr, verbose);
                    {
                        std::lock_guard<std::mutex> lock(mtx);
                        for (auto const & [key, value] : local_result) {
                            auto found = aggregated_result.find(key);
                            if (found == aggregated_result.end()) {
                                aggregated_result.emplace(key, value);
                            } else {
                                found->second.combine(value);
                            }
                        }
                    }
                    promise.set_value();
                } catch (std::runtime_error& e) {
                    fmt::println("Exception in partition {}: {}", partition_nr, e.what());
                    promise.set_exception(std::current_exception());
                }
            });
        }
        for(auto & e : futures) {
            try {
                e.get();
            } catch (std::runtime_error& e) {
                ret = ERROR_FILE_FORMAT;
            } catch (std::exception& e) {
                ret = ERROR_OTHER;
            }
        }
        if (ret != 0)
            exit(ret);

        // convert to a sorted map
        std::map<std::string, statistics, UTF8StringComparator> sorted_map(aggregated_result.begin(), aggregated_result.end());
        // print all collected statistics
        fmt::println(" **** Statistics ***");
        size_t cnt = 0;
        for (auto const &e: sorted_map) {
            fmt::println("{:<30} {:5.1f}|{:5.1f}|{:5.1f}|{:6d}", e.first,
                e.second.min_, e.second.avg(), e.second.max_, e.second.cnt_);
            cnt += e.second.cnt_;
        }
        fmt::println(stderr, "\nCounted {} total measures.", cnt);
    }
    return ret;
}
