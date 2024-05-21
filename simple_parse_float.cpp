#include "simple_parse_float.h"
#include <cctype>

int simple_parse_float(std::string_view const & sv, float* result) {
    auto it = sv.begin();
    while (isspace(*it) && it != sv.end()) ++it;
    auto sign = (it != sv.end() && *it == u8'-') ? (++it, -1.f) : 1.f;
    float f = 0.f;
    for(;it != sv.end() && isdigit(*it); ++it) {
        f *= 10.f;
        f += static_cast<float>(*it - u8'0');
    }
    if (it != sv.end() && *it == u8'.')
        ++it;
    for(float cf = 0.1f; it != sv.end() && isdigit(*it); cf *= 0.1f, ++it) {
        f += static_cast<float>(*it - u8'0') * cf;
    }
    *result = f * sign;
    return it == sv.end() ? 0 : 1;
}
