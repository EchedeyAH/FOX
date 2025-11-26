cmake_minimum_required(VERSION 3.16)
project(ecu_atx1610 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB ECU_SOURCES
    logica_sistema/*.cpp
    comunicacion_can/*.cpp
    adquisicion_datos/*.cpp
    control_vehiculo/*.cpp
    interfaces/*.cpp
)

add_executable(${PROJECT_NAME} ${ECU_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/comunicacion_can
    ${CMAKE_CURRENT_SOURCE_DIR}/adquisicion_datos
    ${CMAKE_CURRENT_SOURCE_DIR}/control_vehiculo
    ${CMAKE_CURRENT_SOURCE_DIR}/interfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/logica_sistema
)

target_sources(${PROJECT_NAME} PRIVATE common/types.hpp common/interfaces.hpp common/logging.hpp)
