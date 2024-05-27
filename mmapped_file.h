//
// Created by arnoldm on 27.04.24.
//

#ifndef MMAPPED_FILE_H
#define MMAPPED_FILE_H

#include <fcntl.h>
#include <filesystem>
#include <string_view>
#include <sys/mman.h>
#include <unistd.h>
#include <utility>

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
        static size_t ps = static_cast<size_t>(sysconf(_SC_PAGESIZE));
        return ps;
    }

    explicit mmapped_file(std::string const &file_name, size_t chunk_size_approx = 1 << 26) // 1 << 26
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
        //std::cerr << __func__ << "(" << off << ")\n";
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
        void *ptr = mmap(nullptr, len, PROT_READ, MAP_SHARED, fd_, static_cast<long>(chunk_start));
        if (MAP_FAILED == ptr) {
            perror(file_name_.c_str());
            throw std::runtime_error("MAP FAILED");
        }
        return mmemory_chunk{ptr, len, chunk_start, initial_offset};
    }

protected:
    void compute_chunks(size_t chunk_size_approx) noexcept {
        auto find_closest_multiple = [](auto n, auto v) {
            size_t result = ((n + v - 1) / v) * v;
            if (result < 2*v) {
                result += v;
            }
            return result;
        };
        chunk_size_ = find_closest_multiple(chunk_size_approx, page_size());
        auto div = std::lldiv(static_cast<long long>(file_size_), static_cast<long long>(chunk_size_));
        chunks_total_ = static_cast<size_t>(div.quot);
        chunks_last_remainder_ = static_cast<size_t>(div.rem);
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

#endif //MMAPPED_FILE_H
