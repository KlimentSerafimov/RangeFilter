cmake_minimum_required(VERSION 3.20)
project(RangeFilter)

set(CMAKE_CXX_STANDARD 11)

include_directories(bloom SuRF)

add_executable(RangeFilter main.cpp
        bloom/bloom_filter.hpp
        Trie.cpp Trie.h
        SuRF/surf_implementation.cpp SuRF/surf_implementation.h
        RangeFilterMultiBloom.cpp RangeFilterMultiBloom.h RangeFilterTemplate.h PointQuery.h bloom/bloom_filter.cpp Frontier.cpp Frontier.h HybridPointQuery.cpp HybridPointQuery.h DatasetAndWorkload.cpp DatasetAndWorkload.h RangeFilterTemplate.cpp PointQuery.cpp SurfPointQuery.h HybridRangeFilterSynthesizer.cpp HybridRangeFilterSynthesizer.h RangeFilterScore.cpp RangeFilterScore.h)

include(${CMAKE_CURRENT_SOURCE_DIR}/SuRF/CMakeLists.txt)