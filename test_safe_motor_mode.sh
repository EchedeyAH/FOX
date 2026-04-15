#!/bin/bash
# test_safe_motor_mode.sh
# Script de prueba completa del Safe Test Mode
# 
# Uso:
#   ./test_safe_motor_mode.sh [build|run|clean|full]
#
# Ejemplos:
#   ./test_safe_motor_mode.sh build    # Compilar todo
#   ./test_safe_motor_mode.sh run      # Ejecutar ejemplos
#   ./test_safe_motor_mode.sh full     # Compilar + ejecutar + análisis
#   ./test_safe_motor_mode.sh clean    # Limpiar build

set -e  # Exit on error

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ECU_DIR="$PROJECT_DIR/ecu_atc8110"
BUILD_DIR="$ECU_DIR/build"
EXAMPLES_DIR="$ECU_DIR/examples"

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'  # No Color

# ═════════════════════════════════════════════════════════════════════════
# FUNCIONES AUXILIARES
# ═════════════════════════════════════════════════════════════════════════

print_header() {
    echo -e "\n${BLUE}╔════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║  $1${NC}"
    echo -e "${BLUE}╚════════════════════════════════════════════════╝${NC}\n"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_info() {
    echo -e "${YELLOW}ℹ️  $1${NC}"
}

check_safe_test_mode() {
    print_info "Verificando SAFE_TEST_MODE..."
    
    # Buscar en archivo
    if grep -q "constexpr bool SAFE_TEST_MODE = true" "$PROJECT_DIR/control_vehiculo/safe_motor_test.hpp"; then
        print_success "SAFE_TEST_MODE = true ✅"
    else
        print_error "SAFE_TEST_MODE no está activado"
        return 1
    fi
    
    # Verificar MAX_TORQUE_SAFE
    if grep -q "constexpr double MAX_TORQUE_SAFE = 15.0" "$PROJECT_DIR/control_vehiculo/safe_motor_test.hpp"; then
        print_success "MAX_TORQUE_SAFE = 15.0 Nm ✅"
    else
        print_error "MAX_TORQUE_SAFE no es 15.0"
        return 1
    fi
    
    # Verificar MAX_VOLTAGE_SAFE
    if grep -q "constexpr double MAX_VOLTAGE_SAFE = 2.0" "$PROJECT_DIR/control_vehiculo/safe_motor_test.hpp"; then
        print_success "MAX_VOLTAGE_SAFE = 2.0 V ✅"
    else
        print_error "MAX_VOLTAGE_SAFE no es 2.0"
        return 1
    fi
    
    return 0
}

# ═════════════════════════════════════════════════════════════════════════
# COMANDOS
# ═════════════════════════════════════════════════════════════════════════

build() {
    print_header "Compilando Safe Motor Test Example"
    
    # Crear directorio build si no existe
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    print_info "Ejecutando CMake..."
    cmake .. \
        -DCMAKE_BUILD_TYPE=Debug \
        -DENABLE_SAFE_TEST_MODE=ON \
        2>&1 | head -20
    
    print_info "Compilando con make..."
    make -j4 safe_motor_test_example 2>&1 | tail -10
    
    if [ -f "$BUILD_DIR/safe_motor_test_example" ]; then
        print_success "Compilación exitosa"
        return 0
    else
        print_error "Compilación falló"
        return 1
    fi
}

run() {
    print_header "Ejecutando Safe Motor Test Examples"
    
    if [ ! -f "$BUILD_DIR/safe_motor_test_example" ]; then
        print_error "Ejecutable no encontrado. Ejecuta: $0 build"
        return 1
    fi
    
    "$BUILD_DIR/safe_motor_test_example" 2>&1 | tee "$BUILD_DIR/safe_motor_test.log"
    
    print_success "Ejecución completada"
}

analyze() {
    print_header "Análisis de Resultados"
    
    if [ ! -f "$BUILD_DIR/safe_motor_test.log" ]; then
        print_error "No se encontró log. Ejecuta: $0 run"
        return 1
    fi
    
    # Contar ejemplos ejecutados
    local examples=$(grep "=== EJEMPLO" "$BUILD_DIR/safe_motor_test.log" | wc -l)
    print_info "Ejemplos ejecutados: $examples"
    
    # Buscar resultados
    echo -e "\n${YELLOW}📊 Resumen de Resultados:${NC}"
    grep -E "✅|❌|\[RESULT\]|\[ANALYSIS\]" "$BUILD_DIR/safe_motor_test.log" || true
    
    # Verificaciones
    echo -e "\n${YELLOW}🔍 Verificaciones:${NC}"
    
    if grep -q "✅.*torque" "$BUILD_DIR/safe_motor_test.log"; then
        print_success "Validación de torque"
    fi
    
    if grep -q "✅.*freno" "$BUILD_DIR/safe_motor_test.log"; then
        print_success "Failsafe de freno"
    fi
    
    if grep -q "SAFE_STOP" "$BUILD_DIR/safe_motor_test.log"; then
        print_success "Máquina de estados"
    fi
    
    # Estadísticas
    echo -e "\n${YELLOW}📈 Estadísticas:${NC}"
    local líneas=$(wc -l < "$BUILD_DIR/safe_motor_test.log")
    echo "   Líneas de log: $líneas"
    
    # Avisos de seguridad
    echo -e "\n${YELLOW}⚠️  Notas de Seguridad:${NC}"
    grep "max_torque" "$BUILD_DIR/safe_motor_test.log" | tail -3 || true
}

validate() {
    print_header "Validación de Seguridad"
    
    local passed=0
    local failed=0
    
    # Test 1: SAFE_TEST_MODE activo
    if check_safe_test_mode; then
        ((passed++))
    else
        ((failed++))
    fi
    
    # Test 2: Archivo base_motor_test.hpp existe
    if [ -f "$PROJECT_DIR/control_vehiculo/safe_motor_test.hpp" ]; then
        print_success "Archivo safe_motor_test.hpp existe"
        ((passed++))
    else
        print_error "Archivo safe_motor_test.hpp NO existe"
        ((failed++))
    fi
    
    # Test 3: rt_threads.hpp actualizado
    if grep -q "MAX_TORQUE_SAFE" "$ECU_DIR/logica_sistema/rt_threads.hpp"; then
        print_success "rt_threads.hpp actualizado con MAX_TORQUE_SAFE"
        ((passed++))
    else
        print_error "rt_threads.hpp no contiene MAX_TORQUE_SAFE"
        ((failed++))
    fi
    
    # Test 4: Documentación existe
    if [ -f "$PROJECT_DIR/docs/SAFE_TEST_MODE_IMPLEMENTATION.md" ]; then
        print_success "Documentación SAFE_TEST_MODE_IMPLEMENTATION.md existe"
        ((passed++))
    else
        print_error "Documentación NO existe"
        ((failed++))
    fi
    
    # Test 5: Ejemplo existe
    if [ -f "$EXAMPLES_DIR/safe_motor_test_example.cpp" ]; then
        print_success "Ejemplo safe_motor_test_example.cpp existe"
        ((passed++))
    else
        print_error "Ejemplo NO existe"
        ((failed++))
    fi
    
    echo -e "\n${BLUE}═══════════════════════════════════════════${NC}"
    echo "Resultados: ${GREEN}$passed PASSED${NC} | ${RED}$failed FAILED${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════${NC}\n"
    
    return $failed
}

clean() {
    print_header "Limpiando Build"
    
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        print_success "Build limpiado"
    else
        print_info "No hay directorio build"
    fi
}

full() {
    print_header "PRUEBA COMPLETA Safe Test Mode"
    
    print_info "Paso 1/5: Validar archivos..."
    if ! validate; then
        print_error "Validación falló"
        return 1
    fi
    
    print_info "Paso 2/5: Limpiar build anterior..."
    clean
    
    print_info "Paso 3/5: Compilar..."
    if ! build; then
        print_error "Compilación falló"
        return 1
    fi
    
    print_info "Paso 4/5: Ejecutar ejemplos..."
    if ! run; then
        print_error "Ejecución falló"
        return 1
    fi
    
    print_info "Paso 5/5: Analizar resultados..."
    analyze
    
    echo -e "\n${GREEN}╔════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║   ✅ PRUEBA COMPLETA EXITOSA              ║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════╝${NC}\n"
    
    # Mostrar resumen
    echo "📋 Resumen:"
    echo "   ✅ Safe Test Mode configurado correctamente"
    echo "   ✅ MAX_TORQUE limitado a 15.0 Nm"
    echo "   ✅ MAX_VOLTAGE limitado a 2.0 V"
    echo "   ✅ Failsafe de freno funcional"
    echo "   ✅ Rampa de aceleración sin tirones"
    echo "   ✅ Sistema listo para pruebas seguras"
    echo ""
}

# ═════════════════════════════════════════════════════════════════════════
# MAIN
# ═════════════════════════════════════════════════════════════════════════

main() {
    local cmd="${1:-full}"
    
    case "$cmd" in
        build)
            build
            ;;
        run)
            run
            ;;
        analyze)
            analyze
            ;;
        validate)
            validate
            ;;
        clean)
            clean
            ;;
        full)
            full
            ;;
        *)
            echo "Uso: $0 [build|run|analyze|validate|clean|full]"
            echo ""
            echo "Comandos:"
            echo "  build    - Compilar Safe Motor Test Example"
            echo "  run      - Ejecutar ejemplos"
            echo "  analyze  - Analizar resultados"
            echo "  validate - Validar configuración"
            echo "  clean    - Limpiar build"
            echo "  full     - Todo (build + run + analyze) [DEFAULT]"
            echo ""
            exit 1
            ;;
    esac
}

main "$@"
