cmake_minimum_required(VERSION 3.20)
project(RangeFilter)

set(CMAKE_CXX_STANDARD 11)

include_directories(bloom surf)

add_executable(RangeFilter main.cpp
        bloom/bloom_filter.hpp
        Trie.cpp Trie.h
        surf/surf_implementation.cpp surf/surf_implementation.h RangeFilterMultiBloom.cpp RangeFilterMultiBloom.h RangeFilterTemplate.h PointQuery.h bloom/bloom_filter.cpp Frontier.cpp Frontier.h)

include(${CMAKE_CURRENT_SOURCE_DIR}/surf/CMakeLists.txt)