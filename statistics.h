//
// Created by arnoldm on 27.04.24.
//

#ifndef STATISTICS_H
#define STATISTICS_H
#include <algorithm>
#include <ostream>

struct statistics {
    explicit statistics(float const value) : min_{value}, max_{value}, sum_{value}, cnt_{1} {
    }

    statistics(statistics const &) = default;

    void add_value(float const &value) noexcept {
        min_ = std::min(value, min_);
        max_ = std::max(value, max_);
        sum_ += value;
        ++cnt_;
    }

    void combine(statistics const & other) {
        cnt_ += other.cnt_;
        min_ = std::min(min_, other.min_);
        max_ = std::max(max_, other.max_);
        sum_ += other.sum_;
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

#endif //STATISTICS_H
