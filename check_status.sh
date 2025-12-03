#!/bin/bash

echo "========================================="
echo "  Estado de la ECU ATC-8110"
echo "========================================="
echo ""

# 1. Verificar proceso
echo "1. Proceso ECU:"
if pgrep -x ecu_atc8110 > /dev/null; then
    echo "   ✅ ECU corriendo (PID: $(pgrep -x ecu_atc8110))"
else
    echo "   ❌ ECU NO está corriendo"
fi
echo ""

# 2. Interfaces CAN
echo "2. Interfaces CAN:"
for iface in can0 can1; do
    if ip link show $iface &>/dev/null; then
        state=$(ip link show $iface | grep -oP 'state \K\w+')
        echo "   $iface: $state"
    else
        echo "   $iface: NO ENCONTRADA"
    fi
done
echo ""

# 3. Estadísticas CAN
echo "3. Mensajes CAN (últimos 5 segundos):"
timeout 5 candump can0,can1 2>/dev/null | wc -l | xargs echo "   Mensajes recibidos:"
echo ""

# 4. Últimos logs
echo "4. Últimos logs:"
tail -n 5 /home/fox/ecu_app/logs/ecu_*.log 2>/dev/null | tail -5
echo ""

echo "========================================="
