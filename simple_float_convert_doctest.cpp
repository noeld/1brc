#include <algorithm>
#include <limits>
#include <random>
#include <ranges>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "simple_parse_float.h"
#include <fmt/core.h>
#include <doctest/doctest.h>

struct test_case {
  std::string_view input_;
  float expected_;
  bool rc_expected_;
};

TEST_CASE("Check simple_parse_float") {
  using namespace std::string_view_literals;
  auto lowest_string = fmt::format("{:.f}", std::numeric_limits<float>::lowest());
  auto max_string = fmt::format("{:.f}", std::numeric_limits<float>::max());
  auto min_string = fmt::format("{:.f}", std::numeric_limits<float>::min());
  test_case tests[] = {
      {""sv, 0.f, false},  // should fail
      {"       "sv, 0.f, false},  // should fail
      {"    .   "sv, 0.f, false},  // should fail
      {"  -  .   "sv, 0.f, false},  // should fail
      {"  -.   "sv, 0.f, false},  // should fail
      {"-"sv, 0.f, false}, // should fail
      {"."sv, 0.f, false}, // should fail
      {"-."sv, 0.f, false}, // should fail
      {"abc"sv, 0.f, false}, // should fail
      {"1 2"sv, 0.f, false}, // should fail
      {"1 .2"sv, 0.f, false}, // should fail
      {"12.34", 12.34f, true},
      {"   12.34   ", 12.34f, true},
      {"   12.34", 12.34f, true},
      {"12.34   ", 12.34f, true},
      {"-1"sv, -1.f, true},
      {"1"sv, 1.f, true},
      {"0"sv, 0.f, true},
      {".2"sv, .2f, true},
      {"-.3"sv, -.3f, true},
      {std::string_view(lowest_string), std::numeric_limits<float>::lowest(),
       true},
      {std::string_view(min_string), std::numeric_limits<float>::min(), true},
      {std::string_view(max_string), std::numeric_limits<float>::max(), true}
    };
  SUBCASE("checking known inputs") {
    for (auto const &e : tests) {
      auto result_code = simple_parse_float(e.input_);
      CHECK(result_code.has_value() == e.rc_expected_);
      if (e.rc_expected_){
            float result_float = result_code.value();
            CHECK(result_float == doctest::Approx(e.expected_));
        }
    }
  }
  SUBCASE("comparing with std::stof") {
    for (auto const &e : tests | std::views::filter([](auto &e) {
                           return e.rc_expected_;
                         })) {
      float result_std;
      int exceptions = 0;
      auto result_code = simple_parse_float(e.input_);
      try {
        result_std = std::stof(std::string(e.input_));
      } catch (std::exception const &e) {
        ++exceptions;
      }
      if (exceptions == 0) {
        CHECK(result_code);
        CHECK(result_std == doctest::Approx(result_code.value()));
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
        std::string str = fmt::format("{:.f}", f);
        auto res = simple_parse_float(str);
        auto res_std = std::stof(std::string(str));
        CHECK(res);
        CHECK(res.value() == doctest::Approx(res_std));
    }
  }

}

TEST_CASE("Check simple_parse_float2") {
  using namespace std::string_view_literals;
  auto lowest_string = fmt::format("{:.f}", std::numeric_limits<float>::lowest());
  auto max_string = fmt::format("{:.f}", std::numeric_limits<float>::max());
  auto min_string = fmt::format("{:.f}", std::numeric_limits<float>::min());
  test_case tests[] = {
      {""sv, 0.f, false},  // should fail
      {"       "sv, 0.f, false},  // should fail
      {"    .   "sv, 0.f, false},  // should fail
      {"  -  .   "sv, 0.f, false},  // should fail
      {"  -.   "sv, 0.f, false},  // should fail
      {"-"sv, 0.f, false}, // should fail
      {"."sv, 0.f, false}, // should fail
      {"-."sv, 0.f, false}, // should fail
      {"abc"sv, 0.f, false}, // should fail
      // {"1 2"sv, 0.f, false}, // should fail
      // {"1 .2"sv, 0.f, false}, // should fail
      {"12.34", 12.34f, true},
      // {"   12.34   ", 12.34f, true},
      // {"   12.34", 12.34f, true},
      // {"12.34   ", 12.34f, true},
      {"-1"sv, -1.f, true},
      {"1"sv, 1.f, true},
      {"0"sv, 0.f, true},
      {".2"sv, .2f, true},
      {"-.3"sv, -.3f, true},
      // {std::string_view(lowest_string), std::numeric_limits<float>::lowest(),
       // true},
      {std::string_view(min_string), std::numeric_limits<float>::min(), true},
      // {std::string_view(max_string), std::numeric_limits<float>::max(), true}
    };
  SUBCASE("checking known inputs") {
    for (auto const &e : tests) {
      auto result_code = simple_parse_float2(e.input_);
      CHECK(result_code.has_value() == e.rc_expected_);
      if (e.rc_expected_){
            float result_float = result_code.value();
            CHECK(result_float == doctest::Approx(e.expected_));
        }
    }
  }
  SUBCASE("comparing with std::stof") {
    for (auto const &e : tests | std::views::filter([](auto &e) {
                           return e.rc_expected_;
                         })) {
      float result_std;
      int exceptions = 0;
      auto result_code = simple_parse_float2(e.input_);
      try {
        result_std = std::stof(std::string(e.input_));
      } catch (std::exception const &e) {
        ++exceptions;
      }
      if (exceptions == 0) {
        CHECK(result_code);
        CHECK(result_std == doctest::Approx(result_code.value()));
      }
    }
  }
  SUBCASE("checking with random numbers against std::stof") {
    static constexpr size_t TEST_CNT = 1000;
    std::mt19937_64 rnd{std::random_device()()};
    union rf {
        float f;
        unsigned long int uli;
    } rff;
    for(size_t i = 0; i < TEST_CNT; ++i) {
        do {
            rff.uli = rnd();
        } while(isnanf(rff.f) || isinff(rff.f));
        float f = std::clamp(rff.f, -9999.f, 9999.f);
        std::string str = fmt::format("{:.f}", f);
        auto res = simple_parse_float2(str);
        auto res_std = std::stof(std::string(str));
        CHECK(res);
        CHECK(res.value() == doctest::Approx(res_std));
    }
  }

}