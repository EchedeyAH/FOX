# CMakeLists.txt para herramientas de diagnóstico

# Herramienta de diagnóstico CAN
add_executable(can_diagnostic
    can_diagnostic.cpp
)

target_link_libraries(can_diagnostic
    PRIVATE
    pthread
)

target_include_directories(can_diagnostic
    PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# Herramienta de diagnóstico de sensores
add_executable(sensor_diagnostic
    sensor_diagnostic.cpp
)

target_link_libraries(sensor_diagnostic
    PRIVATE
    ecu_adquisicion_datos
    ecu_common
    pthread
)

target_include_directories(sensor_diagnostic
    PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# Instalar en directorio tools
install(TARGETS can_diagnostic sensor_diagnostic
    RUNTIME DESTINATION tools
)

