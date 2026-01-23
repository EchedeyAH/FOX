cmake_minimum_required(VERSION 3.10)
project(ecu_atc8110 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Buscar soporte para threads
find_package(Threads REQUIRED)

# ===== PATH DEL DRIVER IXPIO =====
set(IXPIO_ROOT /home/fox/Drivers/PEX-DA16)
set(IXPIO_INCLUDE ${IXPIO_ROOT}/include)

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
add_executable(${PROJECT_NAME}
    logica_sistema/main.cpp
    ${ECU_LIB_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/comunicacion_can
    ${CMAKE_CURRENT_SOURCE_DIR}/adquisicion_datos
    ${CMAKE_CURRENT_SOURCE_DIR}/control_vehiculo
    ${CMAKE_CURRENT_SOURCE_DIR}/interfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/logica_sistema
    ${IXPIO_INCLUDE}          # <-- DRIVER IXPIO
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE Threads::Threads stdc++fs
)

# ===== LIBRERÍAS =====
file(GLOB COMMON_SOURCES common/*.cpp)
file(GLOB ADQ_SOURCES adquisicion_datos/*.cpp)
file(GLOB CAN_SOURCES comunicacion_can/*.cpp)

add_library(ecu_common STATIC ${COMMON_SOURCES})
add_library(ecu_adquisicion_datos STATIC ${ADQ_SOURCES})
add_library(ecu_comunicacion_can STATIC ${CAN_SOURCES})

target_include_directories(ecu_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(ecu_adquisicion_datos PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IXPIO_INCLUDE}          # <-- DRIVER IXPIO
)

target_include_directories(ecu_comunicacion_can PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Tools
add_subdirectory(tools)
