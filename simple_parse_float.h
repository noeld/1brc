//
// Created by arnoldm on 27.04.24.
//

#ifndef SIMPLE_PARSE_FLOAT_H
#define SIMPLE_PARSE_FLOAT_H
#include <optional>
#include <string_view>
#include <array>

/**
 * @brief      parse a float value from a string_view (i.e. no copying of data
 * needed). This function supports the following format written as a regexp:
 * \s*-?\d+(\.\d*)?\s*
 * I.e.:
 * any number of spaces
 * followed by an optional - sign
 * followed by digits
 * followed by an optional .
 * followed by optional digits
 * followed by optional spaces
 *
 * @param      sv      the input string_view
 *
 * @return     returns optional float
 */
auto simple_parse_float(std::string_view const &sv)
    -> std::optional<float>;

auto simple_parse_float2(std::string_view const &sv)
    -> std::optional<float>;

inline auto super_simple_parse_float(std::string_view const &sv)
    -> std::optional<float>
{
    auto it = sv.begin();
    int sign = (it != sv.end() && *it == u8'-') ? (++it, -1) : 1;
    long res = (it[1] == '.') ? (it[0] * 10) + it[2] - ('0' * 11) : it[0] * 100 + it[1] * 10 +it[3] - '0' * 111;
    res *= sign;
    return {(float)res / 10.f};
}
#endif // SIMPLE_PARSE_FLOAT_H
