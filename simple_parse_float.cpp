#include "simple_parse_float.h"
#include <cctype>
#include <optional>

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
