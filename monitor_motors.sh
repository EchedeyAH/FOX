#!/bin/bash
# monitor_motors.sh - Utilidad para verificar el arranque de los motores

echo "===================================================="
echo "   ECU FOX - MONITOR DE ACTIVACIÓN DE MOTORES       "
echo "===================================================="
echo "Instrucciones:"
echo "1. Ejecuta la ECU en otra terminal: sudo ./ecu_atc8110"
echo "2. Observa esta terminal para ver comandos de par"
echo "3. Presiona CTRL+C para detener"
echo "===================================================="

# Función para detener todo al salir
cleanup() {
    echo ""
    echo "Deteniendo monitoreo..."
    kill $CANDUMP_PID 2>/dev/null
    exit
}
trap cleanup SIGINT

# 1. Mostrar logs de la ECU en tiempo real (si rediriges a un archivo)
# Si no usas archivos, se verá en la terminal donde lances la ECU.
# Aquí monitoreamos el tráfico CAN específico (Comandos 0x201-0x204)
echo "Esperando tráfico CAN en emuccan0 (IDs 0x100, 0x201-0x204)..."
candump emuccan0,0100:07FF,0201:07FF,0202:07FF,0203:07FF,0204:07FF &
CANDUMP_PID=$!

# 2. Instrucción de simulación si no hay sensores
echo ""
echo "CONSEJO: Si no tienes pedales, usa esto en OTRA terminal:"
echo "  echo 1 > /tmp/force_brake    # Para pasar a estado OPERANDO"
echo "  echo 0.1 > /tmp/force_accel  # Para dar gas (10% par)"
echo "===================================================="

# Mantener vivo el script
wait $CANDUMP_PID
