#include <iostream>
#include "math_utils.h"
#include "string_utils.h"

int main() {
    std::cout << "=== Math Utils ===" << std::endl;
    std::cout << "add(3, 4) = " << add(3, 4) << std::endl;
    std::cout << "multiply(5, 6) = " << multiply(5, 6) << std::endl;
    std::cout << "factorial(5) = " << factorial(5) << std::endl;

    std::cout << "\n=== String Utils ===" << std::endl;
    std::cout << "to_upper(\"hello\") = " << to_upper("hello") << std::endl;
    std::cout << "to_lower(\"WORLD\") = " << to_lower("WORLD") << std::endl;
    std::cout << "repeat(\"ab\", 3) = " << repeat("ab", 3) << std::endl;

    return 0;
}
