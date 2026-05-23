#include "string_utils.h"
#include <algorithm>

std::string to_upper(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

std::string to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string repeat(const std::string& s, int n) {
    std::string result;
    for (int i = 0; i < n; i++) {
        result += s;
    }
    return result;
}
