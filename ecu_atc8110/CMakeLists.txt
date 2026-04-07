cmake_minimum_required(VERSION 3.10)
project(ecu_atc8110 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Buscar soporte para threads
find_package(Threads REQUIRED)

# ===== PATH DEL DRIVER IXPIO =====
set(IXPIO_ROOT /home/fox/Drivers/PEX-DA16)
set(IXPIO_INCLUDE ${IXPIO_ROOT}/include)

# App sources (excluding common/adquisicion_datos/comunicacion_can)
file(GLOB ECU_APP_SOURCES
    control_vehiculo/*.cpp
    control_vehiculo/*.c
    logica_sistema/supervisor.cpp
    interfaces/cli_interface.cpp
)

# ===== LIBRERÍAS =====
file(GLOB COMMON_SOURCES common/*.cpp)
file(GLOB ADQ_SOURCES adquisicion_datos/*.cpp adquisicion_datos/*.c)
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

# ===== HELPER =====
function(link_ecu_libs target)
    target_link_libraries(${target}
        PRIVATE
        ecu_adquisicion_datos
        ecu_comunicacion_can
        ecu_common
        Threads::Threads
    )
endfunction()

# Main executable
add_executable(${PROJECT_NAME}
    logica_sistema/main.cpp
    ${ECU_APP_SOURCES}
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
    PRIVATE stdc++fs rt
)
link_ecu_libs(${PROJECT_NAME})

# Motor Control Program (simplified main for motor testing)
add_executable(motor_control
    motor_control_main.cpp
    ${ECU_APP_SOURCES}
)

target_include_directories(motor_control PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/comunicacion_can
    ${CMAKE_CURRENT_SOURCE_DIR}/adquisicion_datos
    ${CMAKE_CURRENT_SOURCE_DIR}/control_vehiculo
    ${CMAKE_CURRENT_SOURCE_DIR}/interfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/logica_sistema
    ${IXPIO_INCLUDE}
)

target_link_libraries(motor_control
    PRIVATE stdc++fs
)
link_ecu_libs(motor_control)

# Test Analog Inputs (standalone test for PEX-1202L)
add_executable(test_analog_inputs
    test_analog_inputs.cpp
)

target_include_directories(test_analog_inputs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/adquisicion_datos
    ${IXPIO_INCLUDE}
)

target_link_libraries(test_analog_inputs
    PRIVATE stdc++fs
)
link_ecu_libs(test_analog_inputs)

# Simple ADC Test (no handshake, minimal dependencies)
add_executable(test_simple_adc
    test_simple_adc.cpp
)

target_include_directories(test_simple_adc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/adquisicion_datos
    ${IXPIO_INCLUDE}
)

link_ecu_libs(test_simple_adc)

# Direct Copy Test (read_channels.c ported to C++)
add_executable(test_adc_direct
    test_adc_direct.cpp
)

target_include_directories(test_adc_direct PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/adquisicion_datos
    ${IXPIO_INCLUDE}
)

link_ecu_libs(test_adc_direct)

# Tools
add_subdirectory(tools)
