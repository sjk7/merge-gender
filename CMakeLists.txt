cmake_minimum_required(VERSION 3.21)
project(merge_gender)

set(CMAKE_CXX_STANDARD 20)

add_executable(merge_gender
        merge-gender.cpp)

include_directories(../../utils)

target_link_libraries(merge_gender winmm)