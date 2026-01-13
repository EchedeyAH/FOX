#!/bin/bash
# auto_config_can.sh - Detección automática de puerto CAN (EMUC-B202)

echo "=== AUTO-CONFIG CAN INTERFACE ==="

# Detener demonios previos
echo "Deteniendo instancias previas de emucd_64..."
sudo pkill -9 emucd_64
sleep 2

# Buscar puertos disponibles
PORTS=$(ls /dev/ttyACM* 2>/dev/null)

if [ -z "$PORTS" ]; then
    echo "ERROR: No se encontraron dispositivos /dev/ttyACM*. Verifica conexión USB."
    exit 1
fi

echo "Puertos encontrados: $PORTS"

for PORT in $PORTS; do
    DEVICE_NAME=$(basename $PORT)
    echo "--> Probando en $PORT..."
    
    # Intentar iniciar el demonio con velocidad 7 (500k aprox? o config interna)
    # Nota: -s7 fue usado en config_can_correct.sh
    # Mapeamos 2 interfaces: emuccan0, emuccan1
    sudo /usr/sbin/emucd_64 -s7 -e0 $DEVICE_NAME emuccan0 emuccan1 > /tmp/emuc_auto.log 2>&1 &
    PID=$!
    
    sleep 3
    
    if ps -p $PID > /dev/null; then
        echo "   [SUCCESS] Demonio corriendo en $PORT (PID $PID)"
        
        echo "   Configurando interfaces..."
        sudo ip link set emuccan0 txqueuelen 1000
        sudo ip link set emuccan0 up
        sudo ip link set emuccan1 txqueuelen 1000
        sudo ip link set emuccan1 up
        
        # Verificar si existen
        if ip link show emuccan0 > /dev/null 2>&1; then
            echo "   [OK] emuccan0 activa"
            echo "   [OK] emuccan1 activa"
            echo "Configuración EXITOSA en $PORT"
            
            # Guardar para referencia futura
            echo "Ultimo puerto funcional: $PORT" > /tmp/last_can_port
            exit 0
        else
            echo "   [FAIL] El demonio corre pero las interfaces no subieron."
        fi
    else
        echo "   [FAIL] El demonio murió inmediatamente."
        cat /tmp/emuc_auto.log
    fi
    
    # Limpiar antes del siguiente intento
    sudo pkill -9 emucd_64
    sleep 1
done

echo "ERROR: No se pudo configurar CAN en ningún puerto."
exit 1
