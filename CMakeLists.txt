cmake_minimum_required(VERSION 3.31)
project(codotaku_folly)

set(CMAKE_CXX_STANDARD 26)

find_package(folly CONFIG REQUIRED)

add_executable(codotaku_folly main.cpp)
target_link_libraries(codotaku_folly PRIVATE Folly::folly Folly::folly_deps Folly::follybenchmark Folly::folly_test_util)