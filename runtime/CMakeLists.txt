# Runtime library CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(BasicForgeRuntime C)

set(CMAKE_C_STANDARD 11)

add_library(basicforge_runtime STATIC
    src/bf_string.c
    src/bf_math.c
    src/bf_io.c
    src/bf_exception.c
    src/bf_array.c
    src/bf_object.c
    src/bf_date.c
)

target_include_directories(basicforge_runtime PUBLIC include)

install(TARGETS basicforge_runtime DESTINATION lib)
install(DIRECTORY include/ DESTINATION include/bf)