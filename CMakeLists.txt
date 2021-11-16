cmake_minimum_required(VERSION 3.20)
project(RangeFilter)

set(CMAKE_CXX_STANDARD 14)

include_directories(bloom)

add_executable(RangeFilter main.cpp
        bloom/bloom_filter.hpp)
