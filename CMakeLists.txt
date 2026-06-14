cmake_minimum_required(VERSION 3.20)
project(datacenter LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # for clangd / clang-tidy

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(Threads REQUIRED)

# --- core library: node + scheduler (job is header-only) ---
add_library(datacenter
    src/node/node.cpp
    src/scheduler/scheduler.cpp
)
target_include_directories(datacenter PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_features(datacenter PUBLIC cxx_std_20)
target_link_libraries(datacenter PUBLIC Threads::Threads)

if(MSVC)
    target_compile_options(datacenter PRIVATE /W4)
else()
    target_compile_options(datacenter PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- scheduler daemon executable ---
add_executable(scheduler_daemon apps/scheduler_daemon/main.cpp)
target_link_libraries(scheduler_daemon PRIVATE datacenter)
