# CMakeLists.txt para herramientas de diagnóstico

# Herramienta de diagnóstico CAN
add_executable(can_diagnostic
    tools/can_diagnostic.cpp
)

target_link_libraries(can_diagnostic
    PRIVATE
    pthread
)

target_include_directories(can_diagnostic
    PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# Instalar en directorio tools
install(TARGETS can_diagnostic
    RUNTIME DESTINATION tools
)
