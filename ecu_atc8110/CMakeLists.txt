cmake_minimum_required(VERSION 3.10)
project(ecu_atc8110 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Buscar soporte para threads
find_package(Threads REQUIRED)

# Sources excluding main
file(GLOB ECU_LIB_SOURCES
    comunicacion_can/*.cpp
    adquisicion_datos/*.cpp
    control_vehiculo/*.cpp
    logica_sistema/supervisor.cpp
    interfaces/cli_interface.cpp
    common/*.cpp
)

# Main executable
add_executable(${PROJECT_NAME} logica_sistema/main.cpp ${ECU_LIB_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/comunicacion_can
    ${CMAKE_CURRENT_SOURCE_DIR}/adquisicion_datos
    ${CMAKE_CURRENT_SOURCE_DIR}/control_vehiculo
    ${CMAKE_CURRENT_SOURCE_DIR}/interfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/logica_sistema
)

target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads stdc++fs)

# Crear bibliotecas para reusar en tools
file(GLOB COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp)
file(GLOB ADQ_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/adquisicion_datos/*.cpp)
file(GLOB CAN_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/comunicacion_can/*.cpp)

add_library(ecu_common STATIC ${COMMON_SOURCES})
add_library(ecu_adquisicion_datos STATIC ${ADQ_SOURCES})
add_library(ecu_comunicacion_can STATIC ${CAN_SOURCES})

target_include_directories(ecu_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(ecu_adquisicion_datos PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(ecu_comunicacion_can PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Subdirectorios
add_subdirectory(tools)

# Note: Testing with GoogleTest requires CMake 3.14+
# For now, tests are disabled for Ubuntu 18.04 compatibility
# To run tests, use the standalone test executables in tests/

