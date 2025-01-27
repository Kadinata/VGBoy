# Set the CMake minimum required version
cmake_minimum_required(VERSION 3.27)

project(VGBoy C)

set(CMAKE_C_STANDARD 11)

add_compile_options(-pedantic -Wall -Wextra -Wno-gnu-statement-expression -fsanitize=address,undefined -g3 -O2)
add_link_options(-pedantic -Wall -Wextra -Wno-gnu-statement-expression -fsanitize=address,undefined -g3 -O2)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} main.c)
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})

add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(lib)
