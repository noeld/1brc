//
// Created by arnoldm on 27.04.24.
//

#ifndef SIMPLE_PARSE_FLOAT_H
#define SIMPLE_PARSE_FLOAT_H
#include <optional>
#include <string_view>

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
 * @param      result  a pointer to a float
 *
 * @return     returns optional float
 */
auto simple_parse_float(std::string_view const &sv)
    -> std::optional<float>;

#endif // SIMPLE_PARSE_FLOAT_H
