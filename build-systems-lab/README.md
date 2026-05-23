# Build Systems Lab

Hands-on practice project for GMake, CMake, and Bazel.

The source code is already written. Your job is to write the build files.

## Project Structure

```
src/
  main.cpp           - entry point, uses both util libraries
  math_utils.h/.cpp  - add, multiply, factorial
  string_utils.h/.cpp - to_upper, to_lower, repeat
tests/
  test_main.cpp      - test runner (uses both util libraries)
```

## Expected Output

When you build and run the main binary:
```
=== Math Utils ===
add(3, 4) = 7
multiply(5, 6) = 30
factorial(5) = 120

=== String Utils ===
to_upper("hello") = HELLO
to_lower("WORLD") = world
repeat("ab", 3) = ababab
```

## Exercises

### Exercise 1: Basic Makefile

Write `Makefile` in the project root. Requirements:
- Compile all .cpp files in src/ into a single executable `build/app`
- Use variables for CXX, CXXFLAGS (-Wall -std=c++17), etc.
- `make` builds the app
- `make clean` removes build artifacts

Test: `make && ./build/app`

### Exercise 2: Advanced Makefile

Upgrade your Makefile. Requirements:
- Pattern rules (%.o: %.cpp)
- Separate obj/ and build/ directories
- Auto-dependency generation (-MMD -MP)
- Include the .d files
- Order-only prerequisites for directories
- Also build `build/tests` from tests/test_main.cpp + the util .o files
- `make test` builds and runs the tests
- `make all` builds both app and tests
- Try `make -j4` to verify parallel build works

Test: `make all && ./build/tests`

### Exercise 3: CMake Basics

Write `CMakeLists.txt` in the project root. Requirements:
- cmake_minimum_required (3.16)
- project(BuildSystemsLab)
- set CMAKE_CXX_STANDARD to 17
- Add an executable "app" from the src/ files
- Add an executable "tests" from test_main.cpp + the util files

Build:
```
cmake -S . -B cmake-build
cmake --build cmake-build
./cmake-build/app
./cmake-build/tests
```

### Exercise 4: CMake with Library

Upgrade your CMakeLists.txt. Requirements:
- Create a STATIC library "utils" from math_utils.cpp and string_utils.cpp
- target_include_directories on the library (so consumers find the headers)
- Link "app" against "utils"
- Link "tests" against "utils"
- Add a custom target "run_tests" that runs the test binary

Build:
```
cmake -S . -B cmake-build
cmake --build cmake-build
cmake --build cmake-build --target run_tests
```

### Exercise 5: CMake install rules

Add install() commands to CMakeLists.txt:
- Install the "app" binary to bin/
- Install the headers to include/
- Install the "utils" library to lib/

Test:
```
cmake -S . -B cmake-build -DCMAKE_INSTALL_PREFIX=./install
cmake --build cmake-build
cmake --install cmake-build
ls install/bin install/include install/lib
```

### Exercise 6 (Bonus): Bazel

If you have Bazel installed, create:
- MODULE.bazel (or WORKSPACE) in the project root
- src/BUILD with a cc_library for utils and cc_binary for app
- tests/BUILD with a cc_test for test_main

```
bazel build //src:app
bazel test //tests:all
```

## Tips

- Start each exercise fresh or keep evolving the same file
- Run `make -n` to see what Make WOULD do without executing
- Run `cmake --build cmake-build --verbose` to see actual compiler commands
- If stuck, refer to company-prep/nvidia/01_build_systems.md
