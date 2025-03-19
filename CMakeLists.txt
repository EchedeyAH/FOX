cmake_minimum_required(VERSION 3.10)
project(ECU_FOX C)

set(CMAKE_C_STANDARD 11)

# Directorios de cabeceras
include_directories(include/ecu_fox)
include_directories(include/third_party)

# Recopilamos todos los ficheros fuente de src/
file(GLOB_RECURSE SRC_FILES src/*.c)

add_executable(ecu_fox ${SRC_FILES})
