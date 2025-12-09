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

target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

# Testing
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(test_traction_control tests/test_traction_control.cpp ${ECU_LIB_SOURCES})
target_link_libraries(test_traction_control PRIVATE GTest::gtest_main Threads::Threads)
target_include_directories(test_traction_control PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/control_vehiculo
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

include(GoogleTest)
gtest_discover_tests(test_traction_control)
