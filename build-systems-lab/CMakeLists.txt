cmake_minimum_required(VERSION 3.16)
project(BuildSystemsLab)
set(CMAKE_CXX_STANDARD 17)


add_library(utils STATIC src/math_utils.cpp src/string_utils.cpp)
target_include_directories(utils PUBLIC src)

add_executable(app src/main.cpp)
target_link_libraries(app PRIVATE utils)


add_executable(tests tests/test_main.cpp)
target_link_libraries(tests PRIVATE utils)

install(TARGETS utils DESTINATION lib)
install(TARGETS app DESTINATION bin)
install(FILES src/math_utils.h src/string_utils.h DESTINATION include)
