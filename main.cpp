// https://1brc.dev/#the-challenge
#define _FILE_OFFSET_BITS 64

#include <algorithm>
#include <filesystem>
#include <unordered_set>
#include <iostream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>
#include <sys/mman.h>

/** Input File; UTF-8, UNIX line breaks 0x0a
00000000  4b 61 6e 73 61 73 20 43  69 74 79 3b 2d 30 2e 38  |Kansas City;-0.8|
00000010  0a 44 61 6d 61 73 63 75  73 3b 31 39 2e 38 0a 4b  |.Damascus;19.8.K|
00000020  61 6e 73 61 73 20 43 69  74 79 3b 32 38 2e 30 0a  |ansas City;28.0.|
00000030  4c 61 20 43 65 69 62 61  3b 31 37 2e 30 0a 44 61  |La Ceiba;17.0.Da|
00000040  72 77 69 6e 3b 32 39 2e  31 0a 4e 65 77 20 59 6f  |rwin;29.1.New Yo|
00000050  72 6b 20 43 69 74 79 3b  32 2e 31 0a 4c 69 73 62  |rk City;2.1.Lisb|
...
*/

struct input_file {
    std::string input_name_;
    std::string file_name_;
};

const input_file input_files[] = {
    {.input_name_ = "100", .file_name_ = "measurements.100.txt"},
    {.input_name_ = "1MIO", .file_name_ = "measurements.1E6.txt"},
    {.input_name_ = "1MRD", .file_name_ = "measurements.1E9.txt"}
};

void usage(char *const exe_name) {
    std::cout << "Usage: " << exe_name << " <name>\n"
            << R"*(
with <name> one of:
)*";
    for (auto const &e: input_files) {
        std::cout << "    " << e.input_name_ << " => " << e.file_name_ << '\n';
    }
}

class mmapped_file {
public:
    struct mmemory_chunk {
        mmemory_chunk(void *ptr, size_t len, size_t chunk_start, size_t initial_offset)
            : ptr_{ptr}, len_{len}, chunk_start_(chunk_start), initial_offset_(initial_offset) {
        }

        mmemory_chunk(mmemory_chunk const &) = delete;

        mmemory_chunk &operator=(mmemory_chunk const &) = delete;

        mmemory_chunk &operator=(mmemory_chunk &&other) noexcept {
            this->~mmemory_chunk();
            std::swap(ptr_, other.ptr_);
            std::swap(len_, other.len_);
            std::swap(initial_offset_, other.initial_offset_);
            std::swap(chunk_start_, other.chunk_start_);
            other.ptr_ = nullptr;
            other.initial_offset_ = 0;
            other.len_ = 0;
            other.chunk_start_ = 0;
            return *this;
        };

        mmemory_chunk(mmemory_chunk &&other) noexcept
            : ptr_{other.ptr_}, len_{other.len_}, chunk_start_(other.chunk_start_), initial_offset_(other.initial_offset_) {
            other.ptr_ = nullptr;
            other.len_ = 0;
            other.chunk_start_ = 0;
            other.initial_offset_ = 0;
        }

        ~mmemory_chunk() noexcept {
            if (ptr_ != nullptr)
                munmap(ptr_, len_);
            len_ = 0;
            ptr_ = nullptr;
            chunk_start_ = 0;
        }

        [[nodiscard]] char const * begin() const { return reinterpret_cast<char const *>(ptr_); }
        [[nodiscard]] char const * end() const { return reinterpret_cast<char const *>(ptr_) + len_; }
        [[nodiscard]] std::string_view string_view() const { return std::string_view{begin(), end()}; }

        void *ptr_{nullptr};
        size_t len_{0};
        size_t chunk_start_ {0};
        size_t initial_offset_{0};
    };

    static size_t page_size() {
        static size_t ps = sysconf(_SC_PAGESIZE);
        return ps;
    }

    explicit mmapped_file(std::string const &file_name, size_t chunk_size_approx = 1 << 26)
        : file_name_{file_name} {
        file_size_ = std::filesystem::file_size(file_name_);
        fd_ = open(file_name_.c_str(), O_RDONLY);
        if (fd_ < 0) {
            perror((file_name_.c_str()));
        }
        compute_chunks(chunk_size_approx);
    }

    ~mmapped_file() noexcept {
        close(fd_);
    }

    uintmax_t file_size() const noexcept { return file_size_; }

    operator bool() const noexcept { return fd_ > 0; }

    [[nodiscard]] size_t chunks_total() const noexcept { return chunks_total_; }

    [[nodiscard]] size_t chunk_size() const noexcept { return chunk_size_; }

    /*
    auto get_chunk(size_t n) const -> mmemory_chunk {
        if (n >= chunks_total_)
            throw std::runtime_error("Index out of bounds");
        size_t len = (n < chunks_total_ - 1) ? chunk_size_ : chunks_last_remainder_;
        void *ptr = mmap64(nullptr, len, PROT_READ, MAP_PRIVATE, fd_, n * chunk_size_);
        if (MAP_FAILED == ptr) {
            perror(file_name_.c_str());
            throw std::runtime_error("MAP FAILED");
        }
        return mmemory_chunk{ptr, len, n * chunk_size_, 0};
    }
    */

    auto get_chunk_for_offset(size_t off) const -> mmemory_chunk {
        std::cerr << __func__ << "(" << off << ")\n";
        auto find_closest_multiple = [](auto n, auto v) {
            auto result = ((n + v - 1) / v) * v;
            if (result > n) {
                result -= v;
            }
            return result;
        };
        size_t chunk_start = find_closest_multiple(off, page_size());
        size_t initial_offset = off - chunk_start;
        size_t len = std::min(chunk_size_, file_size_ - chunk_start);
        void *ptr = mmap(nullptr, len, PROT_READ, MAP_PRIVATE, fd_, chunk_start);
        constexpr size_t so = sizeof(off_t);
        if (MAP_FAILED == ptr) {
            perror(file_name_.c_str());
            throw std::runtime_error("MAP FAILED");
        }
        return mmemory_chunk{ptr, len, chunk_start, initial_offset};
    }

protected:
    void compute_chunks(size_t chunk_size_approx) noexcept {
        auto find_closest_multiple = [](auto n, auto v) {
            int result = ((n + v - 1) / v) * v;
            if (result < 2*v) {
                result += v;
            }
            return result;
        };
        chunk_size_ = find_closest_multiple(chunk_size_approx, page_size());
        auto div = std::lldiv(file_size_, chunk_size_);
        chunks_total_ = div.quot;
        chunks_last_remainder_ = div.rem;
        if (div.rem > 0)
            chunks_total_++;
    }

private:
    std::string file_name_;
    uintmax_t file_size_{0}; // size of file in bytes
    int fd_{0};
    size_t chunks_total_;
    size_t chunks_last_remainder_;
    size_t chunk_size_;
};

struct statistics {
    explicit statistics(float const value) : min_{value}, max_{value}, sum_{value}, cnt_{1} {
    }

    void add_value(float const &value) noexcept {
        min_ = std::min(value, min_);
        max_ = std::max(value, max_);
        sum_ += value;
        ++cnt_;
    }

    float min_;
    float max_;
    float sum_;
    unsigned cnt_;

    [[nodiscard]] float avg() const noexcept { return sum_ / static_cast<float>(cnt_); }

    friend std::ostream &operator<<(std::ostream &o, statistics const &s) {
        return o << "min: " << s.min_ << " avg: " << s.avg() << " max: " << s.max_ << " cnt: " << s.cnt_;
    }
};

struct string_hash {
    using hash_type = std::hash<std::string_view>;
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
auto scan_input(mmapped_file const & input, size_t start, size_t end) -> agg_map_type {
    using std::string_view_literals::operator ""sv;
    agg_map_type map;
    constexpr size_t MAX_FIELDS_CNT = 10;
    std::vector<size_t> field_pos(10);
    size_t file_pos = start;
    while (file_pos < end) {
        auto chunk = input.get_chunk_for_offset(file_pos);
        auto sv = chunk.string_view().substr(chunk.initial_offset_);
        field_pos.clear();
        field_pos.push_back(0);
        for (size_t i = 0; i < sv.size(); ++i) {
            if (sv[i] != u8';' && sv[i] != u8'\n')
                continue;
            field_pos.push_back(i + 1);
            if (sv[i] == u8';') {
                if (field_pos.size() >= MAX_FIELDS_CNT) {
                    std::cerr << "Broken format in input file: too many fields at " << (chunk.chunk_start_ + chunk.initial_offset_ + i) << '\n';
                    throw std::runtime_error("Broken format in input file: too many fields");
                }
            }
            if (sv[i] == u8'\n') {
                if (field_pos.size() != 3) {
                    std::cerr << "Broken format in input file: too many fields at " << (chunk.chunk_start_ + chunk.initial_offset_ + i) << '\n';
                    throw std::runtime_error("Broken format in input file: not 2 fields");
                }
                auto station_view = sv.substr(field_pos[0], field_pos[1] - 1 - field_pos[0]);
                auto value_view = sv.substr(field_pos[1], field_pos[2] - 1 - field_pos[1]);
                float float_value = std::stof(std::string(value_view));

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
            }
        }

    }
    return map;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        exit(1);
    }
    std::string selection = argv[1];
    auto input_it = std::find_if(std::begin(input_files), std::end(input_files),
                                 [&](auto const &e) { return e.input_name_ == selection; });
    if (input_it == std::end(input_files)) {
        std::cerr << "Unknown input!\n";
        usage(argv[0]);
        exit(2);
    }
    std::cerr << "Using " << input_it->file_name_ << '\n';
    mmapped_file input(input_it->file_name_);
    if (input) {
        std::cerr << "File has size: " << input.file_size() << '\n';
        std::cerr << "Will divide into " << input.chunks_total() << " chunks of size " << input.chunk_size() << '\n';

        auto agg = scan_input(input, 2163765236ull, input.file_size());

        // print all collected statistics
        std::cerr << " **** Statistics ****\n";
        for (auto const &e: agg) {
            std::cout << e.first << ": " << e.second << '\n';
        }
    }
    return 0;
}
