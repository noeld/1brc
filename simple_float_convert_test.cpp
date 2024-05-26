//
// Created by arnoldm on 27.04.24.
//
#include <random>
#include <sstream>
#include<fmt/core.h>
#include "simple_parse_float.h"
#include<string_view>
#include <bits/stl_algo.h>

bool float_eq(float const & a, float const & b) {
    return std::abs(a - b) <= std::numeric_limits<float>::epsilon();
}

struct test_case {
    std::string_view input_;
    float expected_;
};

int main(int argc, char *argv[]) {
    using namespace std::string_view_literals;
    std::ostringstream oss;
    oss << std::fixed << std::numeric_limits<float>::lowest();
    auto lowest_string = oss.str();
    std::ostringstream oss2;
    oss2 << std::fixed << std::numeric_limits<float>::max();
    auto max_string = oss2.str();
    test_case tests[] = {
        {"   12.34   ", 12.34f},
        //{""sv, 0.f} ,
        {"-1"sv, -1.f} ,
        {"1"sv, 1.f} ,
        {"0"sv, 0.f} ,
        {".2"sv, .2f} ,
        {"-.3"sv, -.3f} ,
        //{"abc"sv, 0.f },
        {std::string_view(lowest_string), std::numeric_limits<float>::lowest() },
        {std::string_view(max_string), std::numeric_limits<float>::max()}
    };
    for (size_t i = 0; auto const & e : tests) {
        float res_f1 = -123.f;
        auto res1 = simple_parse_float(e.input_, &res_f1);
        auto res_std = std::stof(std::move(std::string(e.input_)));
        auto pass = float_eq(e.expected_, res_f1);
        fmt::println("Test {0:2d}: simple_parse_float(\"{1:s}\"){2:<{3}s} = \tres1: {4} res_f1: {5:+010.5f} {6} {7}",
            ++i, e.input_, " ", std::clamp((size_t)1, 20 - e.input_.length(), (size_t)20), res1, res_f1, (pass?"true":"false"), res_std);
    }
    static constexpr size_t TEST_CNT = 100;
    std::mt19937_64 rnd{std::random_device()()};
    static constexpr auto s = sizeof(unsigned long int);
    union rf {
        float f;
        unsigned long int uli;
    } rff;
    for(size_t i = 0; i < TEST_CNT; ++i) {
        rff.uli = rnd();
        float f = rff.f;
        std::ostringstream oss;
        oss << std::fixed << f;
        std::string str = oss.str();
        float result = 0.f;
        auto res = simple_parse_float(str, &result);
        auto res_std = std::stof(std::move(std::string(str)));
        if (res != 0 || !float_eq(result, f)) {
            fmt::println("Input: {:.f} Output {:.15f} retval: {} {:.15f}", f, result, res, res_std);
        }
    }
    return 0;
}
