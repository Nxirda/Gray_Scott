cmake_minimum_required(VERSION 3.18)

project(gray_scott LANGUAGES CXX DESCRIPTION "Gray scott simulation using kiwaku/eve")

set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

add_compile_options(-Wall -Wextra -Werror -Wpedantic -Wconversion -O3 -march=native -g)

find_package(eve CONFIG REQUIRED)
find_package(kiwaku CONFIG REQUIRED)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(simulation simulation.cpp) 
target_link_libraries(simulation PRIVATE kiwaku::kiwaku eve::eve ${SDL2_LIBRARIES})
