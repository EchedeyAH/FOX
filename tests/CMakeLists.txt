cmake_minimum_required(VERSION 3.10)
project(ECU_FOX_Tests C)

set(CMAKE_C_STANDARD 11)

include_directories(../include/ecu_fox)
include_directories(../include/third_party)

file(GLOB_RECURSE TEST_SRC_FILES *.c)

add_executable(test_fox ${TEST_SRC_FILES})
