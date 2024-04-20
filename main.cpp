// https://1brc.dev/#the-challenge
#include <algorithm>
#include <filesystem>
#include <unordered_set>
#include <iostream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <unordered_map>
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

        void *ptr_{nullptr};
        size_t len_{0};
        size_t chunk_start_ {0};
        size_t initial_offset_{0};
    };

    static size_t page_size() {
        static size_t ps = sysconf(_SC_PAGESIZE);
        return ps;
    }

    explicit mmapped_file(std::string const &file_name, size_t chunk_size_approx = 4096 << 10)
        : file_name_{file_name} {
        file_size_ = std::filesystem::file_size(file_name_);
        fd_ = open(file_name_.c_str(), O_RDONLY);
        if (fd_ < 0) {
            perror((file_name_.c_str()));
        }
        compute_chunks();
    }

    ~mmapped_file() noexcept {
        close(fd_);
    }

    uintmax_t file_size() const noexcept { return file_size_; }

    operator bool() const noexcept { return fd_ > 0; }

    [[nodiscard]] size_t chunks_total() const noexcept { return chunks_total_; }

    auto get_chunk(size_t n) const -> mmemory_chunk {
        if (n >= chunks_total_)
            throw std::runtime_error("Index out of bounds");
        size_t len = (n < chunks_total_ - 1) ? chunk_size_ : chunks_last_remainder_;
        void *ptr = mmap(nullptr, len, PROT_READ, MAP_PRIVATE, fd_, n * chunk_size_);
        if (MAP_FAILED == ptr) {
            perror(file_name_.c_str());
            throw std::runtime_error("MAP FAILED");
        }
        return mmemory_chunk{ptr, len, n * chunk_size_, 0};
    }

    auto get_chunk_for_offset(size_t off) const -> mmemory_chunk {
        auto find_closest_multiple = [](auto n, auto v) {
            int result = ((n + v - 1) / v) * v;
            if (result > n) {
                result -= v;
            }
            return result;
        };
        size_t chunk_start = find_closest_multiple(off, page_size());
        size_t initial_offset = off - chunk_start;
        size_t len = std::max(chunk_size_, file_size_ - chunk_start);
        void *ptr = mmap(nullptr, len, PROT_READ, MAP_PRIVATE, fd_, chunk_start);
        if (MAP_FAILED == ptr) {
            perror(file_name_.c_str());
            throw std::runtime_error("MAP FAILED");
        }
        return mmemory_chunk{ptr, len, chunk_start, initial_offset};
    }

protected:
    void compute_chunks() noexcept {
        auto find_closest_multiple = [](auto n, auto v) {
            int result = ((n + v - 1) / v) * v;
            if (result < 0) {
                result += v;
            }
            return result;
        };
        chunk_size_ = find_closest_multiple(file_size_, page_size());
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
    mmapped_file input(input_it->file_name_, 4096);
    if (input) {
        std::cerr << "File has size: " << input.file_size() << '\n';
        std::cerr << "Will divide into " << input.chunks_total() << " chunks.\n";

        std::unordered_map<std::string, statistics, string_hash, std::equal_to<> > map;
        unsigned field_cnt = 0;
        char const *field1_end, *field2_end;

        for (size_t chunk_idx = 0; chunk_idx < input.chunks_total(); ++chunk_idx) {
            auto chunk = input.get_chunk(chunk_idx);
            char const *it = reinterpret_cast<char const *>(chunk.ptr_);
            char const *const start = it;
            char const *const end = it + chunk.len_;

            for (char const *line_it = it; line_it < end; ++line_it) {
                constexpr size_t FIELDS_LEN = 10;
                char const *fields[FIELDS_LEN];
                fields[0] = line_it;
                size_t field_idx = 0;
                for (; line_it < end; ++line_it) {
                    if (*line_it != u8';' && *line_it != u8'\n')
                        continue;

                    ++field_idx;
                    if (field_idx >= FIELDS_LEN) {
                        std::cerr << "Broken format in input file: too many fields at " << (line_it - start) << '\n';
                        throw std::runtime_error("Broken format in input file: too many fields");
                    }
                    fields[field_idx] = line_it + 1;
                    if (*line_it == u8'\n') {
                        if (field_idx != 2) {
                            std::cerr << "Broken format in input file: too many fields at " << (line_it - start) << '\n';
                            throw std::runtime_error("Broken format in input file: not 2 fields");
                        }
                        std::string_view station_view(fields[0], fields[1] - 1);
                        std::string_view value_view(fields[1], fields[2] - 1);
                        float float_value = std::stof(std::string(fields[1], fields[2] - 1));

                        auto found = map.find(station_view);
                        if (found == map.end()) {
                            map.emplace(station_view, float_value);
                        } else {
                            found->second.add_value(float_value);
                        }
                        it = line_it + 1;
                        fields[0] = fields[field_idx];
                        field_idx = 0;

                    }
                }
            }
        }

        // print all collected statistics
        std::cerr << " **** Statistics ****\n";
        for (auto const &e: map) {
            std::cout << e.first << ": " << e.second << '\n';
        }
    }
    return 0;
}
