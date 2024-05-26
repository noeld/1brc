//
// Created by arnoldm on 27.04.24.
//

#ifndef SIMPLE_PARSE_FLOAT_H
#define SIMPLE_PARSE_FLOAT_H
#include <string_view>

/**
 * @brief      parse a float value from a string_view (i.e. no copying of data needed)
 *
 * @param      sv      the input string_view
 * @param      result  a pointer to a float
 *
 * @return     returns 0 on success, 1 on parse error
 */
int simple_parse_float(std::string_view const & sv, float* result);

#endif //SIMPLE_PARSE_FLOAT_H
