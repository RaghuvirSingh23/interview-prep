# Build Systems - GMake, CMake, Bazel

Deep dive into build systems for NVIDIA compiler team interview.

---

## Table of Contents

1. [What is a Build System?](#1-what-is-a-build-system)
2. [GNU Make (GMake)](#2-gnu-make-gmake)
3. [CMake](#3-cmake)
4. [Bazel](#4-bazel)
5. [Comparison](#5-comparison)
6. [Hands-On Exercises](#6-hands-on-exercises)
7. [Interview Questions](#7-interview-questions)

---

## 1. What is a Build System?

A build system automates the process of converting source code into executable programs.

### The Compilation Pipeline (C/C++)

```
Source Files (.c/.cpp)
       │
       ▼
┌──────────────┐
│ PREPROCESSOR │  → Expands #include, #define, macros
│   (cpp)      │  → Output: .i file (translation unit)
└──────────────┘
       │
       ▼
┌──────────────┐
│  COMPILER    │  → Parses, optimizes, generates assembly
│   (cc1)      │  → Output: .s file (assembly)
└──────────────┘
       │
       ▼
┌──────────────┐
│  ASSEMBLER   │  → Converts assembly to machine code
│   (as)       │  → Output: .o file (object file)
└──────────────┘
       │
       ▼
┌──────────────┐
│   LINKER     │  → Combines object files + libraries
│   (ld)       │  → Output: executable or shared library
└──────────────┘
```

```bash
# Manual compilation steps:
gcc -E main.c -o main.i          # Preprocess
gcc -S main.i -o main.s          # Compile to assembly
gcc -c main.s -o main.o          # Assemble to object
gcc main.o utils.o -o myapp      # Link

# Or all at once:
gcc main.c utils.c -o myapp
```

### Why Build Systems?

For a project with 1000+ files:

- Which files changed? (don't rebuild everything)
- What depends on what? (if header changes, recompile dependents)
- What order to build? (libraries before executables)
- How to parallelize? (build independent files simultaneously)

### Dependency Graph

```
          myapp (executable)
          /    \
       main.o  utils.o
       /  \    /   \
   main.c  utils.h  utils.c
```

If `utils.h` changes → rebuild `main.o` and `utils.o` → relink `myapp`.
If `main.c` changes → rebuild only `main.o` → relink `myapp`.

---

## 2. GNU Make (GMake)

### Basics

Makefile syntax:

```makefile
target: prerequisites
	recipe (TAB indented, MUST be a tab, not spaces)
```

### Simple Makefile

```makefile
# Variables
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lm

# Default target (first rule)
myapp: main.o utils.o math.o
	$(CC) $(LDFLAGS) -o myapp main.o utils.o math.o

# Pattern rules
main.o: main.c utils.h math.h
	$(CC) $(CFLAGS) -c main.c

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

math.o: math.c math.h
	$(CC) $(CFLAGS) -c math.c

# Phony targets
.PHONY: clean all

clean:
	rm -f *.o myapp

all: myapp
```

### How Make Decides What to Build

```
1. Parse the Makefile → build dependency DAG (Directed Acyclic Graph)
2. Check timestamps:
   - If target doesn't exist → build it
   - If any prerequisite is NEWER than target → rebuild target
3. Traverse DAG bottom-up, rebuilding what's needed
4. Stop when top-level target is up-to-date

Example:
  main.c modified at 10:05
  main.o built at 10:00
  → main.o is OLDER than main.c → rebuild main.o
  → myapp depends on main.o → relink myapp
  → utils.o not rebuilt (utils.c didn't change)
```

### Automatic Variables

```makefile
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# $@ = target name (e.g., main.o)
# $< = first prerequisite (e.g., main.c)
# $^ = all prerequisites
# $* = stem of pattern match (e.g., main)
# $? = prerequisites newer than target
```

### Pattern Rules

```makefile
# Instead of writing a rule for every .c → .o:
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# This matches ANY .c file and produces corresponding .o
```

### Advanced: Auto-Dependency Generation

Header dependencies are tricky. If `utils.h` changes, Make won't know to rebuild `main.o` unless you tell it.

```makefile
# Generate .d dependency files automatically
SRCS = main.c utils.c math.c
DEPS = $(SRCS:.c=.d)

%.d: %.c
	$(CC) -MM $< > $@

# Include generated dependencies
-include $(DEPS)

# -MM flag: gcc outputs dependencies like:
# main.o: main.c utils.h math.h
```

### Recursive vs Non-Recursive Make

```
RECURSIVE (traditional, problematic):
  project/
  ├── Makefile          ← calls sub-makes
  ├── src/Makefile      ← builds src
  ├── lib/Makefile      ← builds lib
  └── test/Makefile     ← builds tests
  
  # Top-level Makefile:
  all:
      $(MAKE) -C src
      $(MAKE) -C lib
      $(MAKE) -C test
  
  Problem: Each sub-make has incomplete dependency info.
  "Recursive Make Considered Harmful" - Peter Miller

NON-RECURSIVE (better):
  Single Makefile includes sub-makefiles.
  Has complete dependency graph → correct incremental builds.
```

### Parallel Builds

```bash
make -j8        # Run up to 8 jobs in parallel
make -j$(nproc) # Use all CPU cores

# Make automatically parallelizes independent targets
# If A depends on B, B builds first
# If A and C are independent, they build simultaneously
```

### Common Pitfalls

```makefile
# 1. Spaces instead of tabs (WILL FAIL SILENTLY)
target:
    command    # This is spaces - WRONG
	command    # This is a tab - CORRECT

# 2. Missing dependencies
main.o: main.c          # WRONG: forgot utils.h
main.o: main.c utils.h  # CORRECT

# 3. Not using .PHONY
clean:                   # If file named "clean" exists, won't run
	rm -f *.o

.PHONY: clean            # FIX: declare as phony
clean:
	rm -f *.o
```

### Make Functions

```makefile
SRCS = $(wildcard src/*.c)          # All .c files in src/
OBJS = $(patsubst %.c,%.o,$(SRCS)) # Replace .c with .o
DIRS = $(sort $(dir $(SRCS)))       # Unique directories

# String functions
$(subst from,to,text)     # Replace
$(filter %.c,$(FILES))    # Filter matching
$(filter-out %.h,$(FILES))# Filter non-matching
$(shell ls src/)           # Run shell command
$(foreach var,list,text)   # Loop
$(if condition,then,else)  # Conditional
```

### Real-World Makefile (C++ Project)

```makefile
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS  = -lpthread

SRC_DIR  = src
OBJ_DIR  = build
BIN_DIR  = bin

SRCS     = $(wildcard $(SRC_DIR)/*.cpp)
OBJS     = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS     = $(OBJS:.o=.d)
TARGET   = $(BIN_DIR)/myapp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

-include $(DEPS)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
```

---

## 3. CMake

### What is CMake?

CMake is NOT a build system. It's a **build system generator**.

```
CMakeLists.txt → CMake → Makefile (or Ninja, or VS Project)
                              ↓
                           make/ninja → executable
```

### Why CMake Over Make?


| Aspect               | Make                | CMake                                  |
| -------------------- | ------------------- | -------------------------------------- |
| Platform             | Unix-focused        | Cross-platform (Linux, Windows, macOS) |
| Dependencies         | Manual              | Auto-detection (find_package)          |
| Out-of-source builds | Hacky               | Native support                         |
| IDE integration      | None                | Generates VS, Xcode, etc.              |
| Complexity           | Gets messy at scale | Better structure                       |


### Basic CMakeLists.txt

```cmake
# Minimum CMake version
cmake_minimum_required(VERSION 3.16)

# Project name, version, languages
project(MyApp VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create executable from sources
add_executable(myapp main.cpp utils.cpp math.cpp)

# Add compiler flags
target_compile_options(myapp PRIVATE -Wall -Wextra)

# Link libraries
target_link_libraries(myapp PRIVATE pthread)
```

### Building with CMake

```bash
# Out-of-source build (best practice)
mkdir build && cd build
cmake ..                    # Generate Makefile
make -j$(nproc)             # Build

# Or with Ninja (faster than Make)
cmake -G Ninja ..
ninja

# Build types
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
```

### Libraries

```cmake
# Static library
add_library(mylib STATIC lib.cpp)

# Shared library
add_library(mylib SHARED lib.cpp)

# Header-only (interface) library
add_library(mylib INTERFACE)

# Link library to executable
target_link_libraries(myapp PRIVATE mylib)
```

### Target Properties (Modern CMake)

```cmake
# PUBLIC: propagated to dependents
# PRIVATE: only for this target
# INTERFACE: only for dependents (not this target)

add_library(mylib STATIC lib.cpp)
target_include_directories(mylib
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include  # users of mylib get this
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src      # only mylib uses this
)

target_compile_definitions(mylib PUBLIC USE_FEATURE_X)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE mylib)
# myapp automatically gets mylib's PUBLIC include dirs and definitions
```

### Finding External Packages

```cmake
# Find system-installed packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost 1.70 REQUIRED COMPONENTS filesystem system)

target_link_libraries(myapp PRIVATE
    Threads::Threads
    OpenSSL::SSL
    Boost::filesystem
)
```

### Multi-Directory Project

```
project/
├── CMakeLists.txt          ← Top level
├── src/
│   ├── CMakeLists.txt
│   └── main.cpp
├── lib/
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── mylib.h
│   └── mylib.cpp
└── tests/
    ├── CMakeLists.txt
    └── test_main.cpp
```

```cmake
# project/CMakeLists.txt (top level)
cmake_minimum_required(VERSION 3.16)
project(MyProject)
add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(tests)

# project/lib/CMakeLists.txt
add_library(mylib mylib.cpp)
target_include_directories(mylib PUBLIC include)

# project/src/CMakeLists.txt
add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE mylib)

# project/tests/CMakeLists.txt
enable_testing()
add_executable(test_main test_main.cpp)
target_link_libraries(test_main PRIVATE mylib)
add_test(NAME MyTest COMMAND test_main)
```

### CMake Variables & Conditionals

```cmake
# Set variables
set(MY_VAR "hello")
set(SRC_FILES main.cpp utils.cpp)
option(ENABLE_TESTS "Build tests" ON)

# Conditionals
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(myapp PRIVATE DEBUG_MODE)
endif()

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()

# Platform detection
if(WIN32)
    # Windows
elseif(APPLE)
    # macOS
elseif(UNIX)
    # Linux/Unix
endif()
```

### Custom Commands & Targets

```cmake
# Run a command during build
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/generated.h
    COMMAND python3 ${CMAKE_SOURCE_DIR}/gen.py > ${CMAKE_BINARY_DIR}/generated.h
    DEPENDS gen.py
    COMMENT "Generating header..."
)

# Custom target (always runs)
add_custom_target(format
    COMMAND clang-format -i ${ALL_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-format..."
)
```

### Install Rules

```cmake
install(TARGETS myapp DESTINATION bin)
install(TARGETS mylib DESTINATION lib)
install(FILES include/mylib.h DESTINATION include)

# Then: cmake --install build/ --prefix /usr/local
```

---

## 4. Bazel

### What is Bazel?

Google's build system. Key differences from Make/CMake:

- **Hermetic builds**: Build output depends ONLY on declared inputs
- **Reproducible**: Same inputs → same output, every time
- **Remote caching**: Share build results across team
- **Remote execution**: Build on cloud machines
- **Multi-language**: C++, Java, Python, Go, etc. in same project

### Core Concepts

```
WORKSPACE:  Root of project (WORKSPACE or WORKSPACE.bazel file)
PACKAGE:    Directory with a BUILD file
TARGET:     Buildable unit defined in BUILD file
LABEL:      Unique identifier for a target: //path/to:target
RULE:       Function that creates a target (cc_binary, cc_library, etc.)
```

### Project Structure

```
myproject/
├── WORKSPACE              ← Project root marker
├── BUILD                  ← Root package
├── src/
│   ├── BUILD             ← src package
│   ├── main.cc
│   └── utils.cc
├── lib/
│   ├── BUILD             ← lib package
│   ├── mylib.h
│   └── mylib.cc
└── tests/
    ├── BUILD
    └── test_main.cc
```

### WORKSPACE File

```python
# WORKSPACE
workspace(name = "myproject")

# Load external dependency rules
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# External dependency (e.g., Google Test)
http_archive(
    name = "com_google_googletest",
    urls = ["https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz"],
    strip_prefix = "googletest-1.14.0",
)
```

### BUILD Files

```python
# lib/BUILD
cc_library(
    name = "mylib",
    srcs = ["mylib.cc"],
    hdrs = ["mylib.h"],
    visibility = ["//visibility:public"],
)

# src/BUILD
cc_binary(
    name = "myapp",
    srcs = ["main.cc", "utils.cc"],
    deps = ["//lib:mylib"],
)

# tests/BUILD
cc_test(
    name = "test_main",
    srcs = ["test_main.cc"],
    deps = [
        "//lib:mylib",
        "@com_google_googletest//:gtest_main",
    ],
)
```

### Bazel Commands

```bash
# Build a target
bazel build //src:myapp

# Build everything
bazel build //...

# Run a binary
bazel run //src:myapp

# Run tests
bazel test //tests:test_main
bazel test //...              # all tests

# Query dependency graph
bazel query 'deps(//src:myapp)'
bazel query 'rdeps(//..., //lib:mylib)'   # reverse deps

# Clean
bazel clean
bazel clean --expunge   # remove entire output base

# Build with specific config
bazel build //src:myapp --config=release
bazel build //src:myapp -c dbg    # debug mode
bazel build //src:myapp -c opt    # optimized
```

### Labels (How Targets Are Referenced)

```
//path/to/package:target_name

Examples:
  //src:myapp           → src/BUILD, target "myapp"
  //lib:mylib           → lib/BUILD, target "mylib"
  //tests:test_main     → tests/BUILD, target "test_main"
  @gtest//:gtest_main   → external dep "gtest", target "gtest_main"
  :local_target         → same BUILD file
```

### How Bazel Achieves Hermeticity

```
1. SANDBOXING: Each action runs in isolated sandbox
   - Only declared inputs are visible
   - Can't accidentally depend on system files
   
2. CONTENT-BASED HASHING: Cache key = hash(inputs + command)
   - Not timestamp-based like Make
   - Changing a comment doesn't trigger rebuild (if .o unchanged)
   
3. STRICT DEPENDENCY DECLARATION:
   - Must declare ALL dependencies
   - Undeclared deps → build error in sandbox

4. TOOLCHAIN RESOLUTION:
   - Compiler version is part of build inputs
   - Different compiler → different cache key
```

### Bazel vs Make: Incremental Builds

```
MAKE: timestamp-based
  - File modified → rebuild
  - "touch main.c" → rebuilds even if content unchanged
  - Can't cache across machines

BAZEL: content-hash-based
  - hash(file content) → rebuild only if content changes
  - "touch main.c" → NO rebuild (content unchanged)
  - Cache shareable across machines (remote cache)
```

### Visibility

```python
# Only this package can use it
cc_library(
    name = "internal_lib",
    visibility = ["//visibility:private"],   # default
)

# Anyone can use it
cc_library(
    name = "public_lib",
    visibility = ["//visibility:public"],
)

# Specific packages
cc_library(
    name = "restricted_lib",
    visibility = ["//src:__pkg__", "//tests:__pkg__"],
)
```

### Platforms and Toolchains

```python
# .bazelrc
build --crosstool_top=//toolchain:my_toolchain
build --cpu=k8

# Register toolchain
register_toolchains("//toolchain:my_cc_toolchain")

# Cross-compilation
bazel build //src:myapp --platforms=//platforms:aarch64_linux
```

### Genrules (Custom Build Steps)

```python
genrule(
    name = "generate_header",
    srcs = ["schema.proto"],
    outs = ["schema.pb.h"],
    cmd = "protoc --cpp_out=$(@D) $<",
    tools = ["@protobuf//:protoc"],
)
```

### Remote Caching

```bash
# Share build cache across team
bazel build //... --remote_cache=http://cache-server:8080

# How it works:
# 1. Compute cache key = hash(inputs + command)
# 2. Check remote cache for key
# 3. If hit → download result (skip build)
# 4. If miss → build locally → upload result
```

---

## 5. Comparison


| Aspect             | GMake                   | CMake                     | Bazel                      |
| ------------------ | ----------------------- | ------------------------- | -------------------------- |
| **Type**           | Build system            | Build system generator    | Build system               |
| **Config file**    | Makefile                | CMakeLists.txt            | BUILD + WORKSPACE          |
| **Language**       | Make syntax             | CMake DSL                 | Starlark (Python-like)     |
| **Incremental**    | Timestamp-based         | Timestamp-based           | Content-hash-based         |
| **Hermeticity**    | None                    | None                      | Sandboxed                  |
| **Remote cache**   | No                      | No                        | Yes (built-in)             |
| **Cross-platform** | Unix-focused            | Yes                       | Yes                        |
| **Multi-language** | Any (manual)            | C/C++ focused             | C++, Java, Python, Go, ... |
| **Parallelism**    | make -j8                | make -j8 / ninja          | Automatic                  |
| **Learning curve** | Medium                  | Medium                    | Steep                      |
| **Scale**          | Small-medium            | Medium-large              | Massive (Google-scale)     |
| **IDE support**    | None                    | Good (generates projects) | Limited                    |
| **Best for**       | Simple projects, legacy | Cross-platform C++        | Monorepos, large codebases |


### When to Use What

```
Small C project (< 20 files)     → Makefile
Cross-platform C++ library       → CMake
Large monorepo, multiple langs   → Bazel
Legacy codebase                  → Whatever it already uses
NVIDIA compiler team             → Likely CMake or Bazel (possibly Make for legacy)
```

---

## 6. Hands-On Exercises

### Exercise 1: Makefile From Scratch

Create these files and build with Make:

**src/main.c:**

```c
#include <stdio.h>
#include "utils.h"
#include "math_ops.h"

int main() {
    printf("Sum: %d\n", add(3, 4));
    printf("Upper: %s\n", to_upper("hello"));
    return 0;
}
```

**src/utils.h:**

```c
#ifndef UTILS_H
#define UTILS_H
char* to_upper(const char* str);
#endif
```

**src/utils.c:**

```c
#include "utils.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

char* to_upper(const char* str) {
    char* result = malloc(strlen(str) + 1);
    for (int i = 0; str[i]; i++) result[i] = toupper(str[i]);
    result[strlen(str)] = '\0';
    return result;
}
```

**src/math_ops.h / math_ops.c:**

```c
// math_ops.h
#ifndef MATH_OPS_H
#define MATH_OPS_H
int add(int a, int b);
#endif

// math_ops.c
#include "math_ops.h"
int add(int a, int b) { return a + b; }
```

**TODO:** Write a Makefile that:

1. Compiles each .c to .o
2. Links into executable
3. Has `clean` target
4. Uses pattern rules
5. Supports `make -j4`

### Exercise 2: CMake Multi-Library Project

Convert the above to CMake:

1. `math_ops` as a static library
2. `utils` as a static library
3. `main` as executable linking both
4. Add a test executable
5. Support Debug/Release builds

### Exercise 3: Bazel Build

Convert to Bazel:

1. Create WORKSPACE file
2. Create BUILD files for each package
3. Proper visibility rules
4. Add Google Test for testing

---

## 7. Interview Questions

### GMake

**Q: How does Make decide what to rebuild?**

> Compares timestamps of target vs prerequisites. If any prerequisite is newer than target (or target doesn't exist), the target's recipe is executed. Traverses dependency DAG bottom-up.

**Q: What is the difference between `=` and `:=` in Make?**

> `=` is recursive (lazy evaluation, expanded when used). `:=` is simple (immediate evaluation, expanded when defined). `=` can cause infinite loops, `:=` is predictable.

**Q: Explain `$@`, `$<`, `$^`.**

> `$@` = target, `$<` = first prerequisite, `$^` = all prerequisites. Used in pattern rules to write generic recipes.

**Q: What is "recursive make considered harmful"?**

> Recursive make (calling make from make in subdirectories) breaks the dependency graph. Each sub-make has incomplete information, leading to incorrect incremental builds and missed dependencies. Non-recursive make with a single dependency graph is more correct.

**Q: How would you debug a Makefile?**

> `make -n` (dry run, show commands without executing), `make -d` (debug info), `make -p` (print database of rules), `$(info variable=$(VAR))` (print variable value).

### CMake

**Q: What is the difference between PUBLIC, PRIVATE, INTERFACE in CMake?**

> PUBLIC: applied to target AND propagated to dependents. PRIVATE: applied to target only. INTERFACE: propagated to dependents but not applied to target itself. Controls include paths, compile definitions, and link libraries.

**Q: What is an out-of-source build?**

> Building in a separate directory from source. `mkdir build && cd build && cmake ..`. Keeps source tree clean, allows multiple build configurations (Debug, Release) simultaneously.

**Q: How does find_package work?**

> Searches for a Find.cmake module or Config.cmake file. Checks standard paths, CMAKE_PREFIX_PATH, and package-specific hints. Sets variables and imported targets that you can link against.

### Bazel

**Q: How is Bazel different from Make?**

> Hermetic (sandboxed builds), content-hash-based caching (not timestamps), remote caching/execution, strict dependency declaration, reproducible builds. Make is simpler but doesn't guarantee reproducibility.

**Q: What is hermeticity and why does it matter?**

> Build output depends ONLY on declared inputs. No implicit dependencies on system state. Ensures reproducibility: same inputs → same output on any machine. Enables remote caching and distributed builds.

**Q: You're migrating from Make to Bazel. What challenges do you face?**

> (Your Cisco experience!) Mapping implicit Make dependencies to explicit Bazel deps. Handling generated files. Converting recursive Makefiles to BUILD files. Dealing with non-hermetic tools. Training the team. Running both systems in parallel during migration.

**Q: Explain Bazel's build phases.**

> 1. Loading: Parse WORKSPACE and BUILD files, evaluate Starlark
> 2. Analysis: Build action graph from targets and rules
> 3. Execution: Run actions (compile, link) with caching and sandboxing

**Q: How does Bazel remote caching work?**

> Each action has a cache key = hash(all inputs + command + tool versions). Before executing, check remote cache. If hit, download result. If miss, execute locally and upload result. Entire team shares cache → first build populates, subsequent builds are fast.

