#include <math.h>
#include <random>
#include <ranges>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "simple_parse_float.h"
#include <doctest/doctest.h>

struct test_case {
  std::string_view input_;
  float expected_;
  int rc_expected_;
};

TEST_CASE("Compare simple_parse_float with std::stof") {
  using namespace std::string_view_literals;
  std::ostringstream oss;
  oss << std::fixed << std::numeric_limits<float>::lowest();
  auto lowest_string = oss.str();
  std::ostringstream oss2;
  oss2 << std::fixed << std::numeric_limits<float>::max();
  auto max_string = oss2.str();
  test_case tests[] = {
      {"   12.34   ", 12.34f, 1},
      {""sv, 0.f, 0},
      {"-1"sv, -1.f, 0},
      {"1"sv, 1.f, 0},
      {"0"sv, 0.f, 0},
      {".2"sv, .2f, 0},
      {"-.3"sv, -.3f, 0},
      {"abc"sv, 0.f, 1},
      {std::string_view(lowest_string), std::numeric_limits<float>::lowest(),
       0},
      {std::string_view(max_string), std::numeric_limits<float>::max(), 0}};
  SUBCASE("checking known inputs") {
    for (auto const &e : tests) {
      float result_float = -123.f;
      auto result_code = simple_parse_float(e.input_, &result_float);
      CHECK(result_code == e.rc_expected_);
      if (e.rc_expected_ == 0){
            CHECK(result_float == doctest::Approx(e.expected_));
        }
    }
  }
  SUBCASE("comparing with std::stof") {
    for (auto const &e : tests | std::views::filter([](auto &e) {
                           return e.rc_expected_ == 0;
                         })) {
      float result_float = -123.f, result_std;
      int exceptions = 0;
      auto result_code = simple_parse_float(e.input_, &result_float);
      try {
        result_std = std::stof(std::string(e.input_));
      } catch (std::exception const &e) {
        ++exceptions;
      }
      if (exceptions == 0) {
        CHECK(result_std == doctest::Approx(result_float));
        CHECK(result_code == 0);
      }
    }
  }
  SUBCASE("checking with random numbers against std::stof") {
    static constexpr size_t TEST_CNT = 100'000;
    std::mt19937_64 rnd{std::random_device()()};
    union rf {
        float f;
        unsigned long int uli;
    } rff;
    for(size_t i = 0; i < TEST_CNT; ++i) {
        do {
            rff.uli = rnd();
        } while(isnanf(rff.f) || isinff(rff.f));
        float f = rff.f;
        std::ostringstream oss;
        oss << std::fixed << f;
        std::string str = oss.str();
        float result = 0.f;
        auto res = simple_parse_float(str, &result);
        auto res_std = std::stof(std::string(str));
        CHECK(res == 0);
        CHECK(result == doctest::Approx(res_std));
    }
  }

}