#!/bin/bash
# Script de compilación y ejecución para control de motores
# Uso: ./run_motor_control.sh

set -e  # Salir en caso de error

echo "========================================"
echo " FOX ECU - Control de Motores"
echo "========================================"
echo ""

# Verificar que estamos en la ECU
if [ ! -d "/home/fox" ]; then
    echo "ERROR: Este script debe ejecutarse en la ECU (no en PC local)"
    echo "Conecta a la ECU: ssh fox@193.147.165.236"
    exit 1
fi

# Directorio del proyecto
PROJECT_DIR="/home/fox/ecu_atc8110"

if [ ! -d "$PROJECT_DIR" ]; then
    echo "ERROR: Directorio $PROJECT_DIR no encontrado"
    echo "Transfiere el código primero: scp -r ecu_atc8110 fox@193.147.165.236:/home/fox/"
    exit 1
fi

cd "$PROJECT_DIR"

echo "=== Paso 1: Compilando ecu_atc8110 ==="
mkdir -p build && cd build
cmake .. && make -j4 motor_control
echo "✓ Compilación exitosa"
echo ""

echo "=== Paso 2: Configurando interfaces CAN ==="
cd "$PROJECT_DIR"
sudo ./scripts/setup_can.sh --real
echo "✓ CAN configurado"
echo ""

echo "=== Paso 3: Iniciando control de motores ==="
echo ""
echo "IMPORTANTE:"
echo "- Asegúrate de que el vehículo esté elevado o en espacio seguro"
echo "- Pisa el freno para iniciar"
echo "- Usa Ctrl+C para detener"
echo ""
read -p "¿Continuar? (s/N): " -n 1 -r
echo ""
if [[ ! $REPLY =~ ^[SsYy]$ ]]; then
    echo"Abortado por el usuario"
    exit 0
fi

echo ""
echo "========================================"
cd build
sudo ./motor_control
