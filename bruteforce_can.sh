#!/bin/bash
# Script de fuerza bruta para encontrar la configuración correcta

cat > /tmp/bruteforce_can.sh << 'EOF'
#!/bin/bash
pkill -9 emucd_64
sleep 1

# Puertos a probar
PORTS=$(ls /dev/ttyACM*)

# Baud rates a probar (7=500k, 9=1M, 6=250k)
RATES="7 9 6"

for PORT in $PORTS; do
    echo "=== Probando puerto $PORT ==="
    
    for RATE in $RATES; do
        echo "  - Probando Baud Rate $RATE..."
        
        # Intentar iniciar
        /usr/sbin/emucd_64 -s$RATE -e0 $PORT emuccan0 emuccan1 emuccan2 emuccan3 > /tmp/emuc_bf.log 2>&1 &
        PID=$!
        sleep 2
        
        if ps -p $PID > /dev/null; then
            echo "SUCCESS!!!! Encontrado en $PORT con Rate $RATE"
            echo "Levantando interfaces..."
            ip link set emuccan0 txqueuelen 1000
            ip link set emuccan0 up
            ip link set emuccan1 txqueuelen 1000
            ip link set emuccan1 up
            ip link show | grep emuccan
            exit 0
        else
            echo "    Fallo. Log:"
            cat /tmp/emuc_bf.log
        fi
    done
done

echo "FAILED: No se encontró configuración válida"
exit 1
EOF

chmod +x /tmp/bruteforce_can.sh
echo 'FOX' | sudo -S /tmp/bruteforce_can.sh
