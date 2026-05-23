#include <iostream>
#include <cassert>
#include "math_utils.h"
#include "string_utils.h"

int tests_passed = 0;
int tests_failed = 0;

void check(bool condition, const char* name) {
    if (condition) {
        std::cout << "  PASS: " << name << std::endl;
        tests_passed++;
    } else {
        std::cout << "  FAIL: " << name << std::endl;
        tests_failed++;
    }
}

void test_math() {
    std::cout << "\n[Math Tests]" << std::endl;
    check(add(2, 3) == 5, "add(2,3) == 5");
    check(add(-1, 1) == 0, "add(-1,1) == 0");
    check(add(0, 0) == 0, "add(0,0) == 0");
    check(multiply(3, 4) == 12, "multiply(3,4) == 12");
    check(multiply(0, 5) == 0, "multiply(0,5) == 0");
    check(multiply(-2, 3) == -6, "multiply(-2,3) == -6");
    check(factorial(0) == 1, "factorial(0) == 1");
    check(factorial(1) == 1, "factorial(1) == 1");
    check(factorial(5) == 120, "factorial(5) == 120");
}

void test_strings() {
    std::cout << "\n[String Tests]" << std::endl;
    check(to_upper("hello") == "HELLO", "to_upper(hello)");
    check(to_upper("") == "", "to_upper(empty)");
    check(to_lower("WORLD") == "world", "to_lower(WORLD)");
    check(repeat("ab", 3) == "ababab", "repeat(ab, 3)");
    check(repeat("x", 0) == "", "repeat(x, 0)");
    check(repeat("", 5) == "", "repeat(empty, 5)");
}

int main() {
    test_math();
    test_strings();

    std::cout << "\n=============================\n";
    std::cout << "Results: " << tests_passed << " passed, "
              << tests_failed << " failed" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
