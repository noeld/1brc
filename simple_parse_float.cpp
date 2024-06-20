#include "simple_parse_float.h"
#include <cctype>
#include <optional>
#include <array>

auto simple_parse_float(std::string_view const &sv) -> std::optional<float> {
  auto it = sv.begin();
  while (isspace(*it) && it != sv.end())
    ++it;
  auto sign = (it != sv.end() && *it == u8'-') ? (++it, -1.f) : 1.f;
  auto const start_leading_digits = it;
  float f = 0.f;
  for (; it != sv.end() && isdigit(*it); ++it) {
    f *= 10.f;
    f += static_cast<float>(*it - u8'0');
  }
  auto const leading_digits = it - start_leading_digits;
  if (it != sv.end() && *it == u8'.')
    ++it;
  auto const start_trailing_digits = it;
  for (float cf = 0.1f; it != sv.end() && isdigit(*it); cf *= 0.1f, ++it) {
    f += static_cast<float>(*it - u8'0') * cf;
  }
  auto const trailing_digits = it - start_trailing_digits;
  while (isspace(*it) && it != sv.end())
    ++it;
  if (it == sv.end() && (leading_digits + trailing_digits > 0))
    return std::make_optional<float>(f * sign);
  else
    return {};
}

auto simple_parse_float2(std::string_view const &sv) -> std::optional<float> {
  static constexpr std::array<float, 13> div = { 1.f, 0.1f, 0.01f, 0.001f, 0.0001f, 0.00001f,
  0.000001f, 0.0000001f, 0.00000001f, 0.000000001f, 0.0000000001f,
  0.00000000001f, 0.000000000001f };
  auto it = sv.begin();
  // while (isspace(*it) && it != sv.end())
  //   ++it;
  int sign = (it != sv.end() && *it == u8'-') ? (++it, -1) : 1;
  auto const start_leading_digits = it;
  long f = 0;
  for (; it != sv.end() && isdigit(*it); ++it) {
    f *= 10;
    f += static_cast<int>(*it - u8'0');
  }
  auto const leading_digits = it - start_leading_digits;
  if (it != sv.end() && *it == u8'.')
    ++it;
  auto const start_trailing_digits = it;
  for (; it != sv.end() && isdigit(*it); ++it) {
    f *= 10;
    f += static_cast<int>(*it - u8'0');
  }
  auto const trailing_digits = it - start_trailing_digits;
  // while (isspace(*it) && it != sv.end())
  //   ++it;
  if ((leading_digits + trailing_digits > 0))
    return std::make_optional<float>(((float)(f) * div[(size_t)std::min(trailing_digits, 13l)]) * (float)sign);
  else
    return {};
}
