#!/bin/bash
# Script FINAL de restauración CAN

cat > /tmp/restore_final.sh << 'EOF'
#!/bin/bash
echo "=== 1. Limpieza total ==="
pkill -9 emucd_64
modprobe -r emuc2socketcan
sleep 1
modprobe emuc2socketcan
sleep 2

echo "=== 2. Buscando puertos ==="
PORTS=$(ls /dev/ttyACM*)
echo "Puertos encontrados: $PORTS"

for PORT in $PORTS; do
    echo ">>> Probando $PORT <<<"
    /usr/sbin/emucd_64 -s7 -e0 $PORT emuccan0 emuccan1 emuccan2 emuccan3 > /tmp/emuc_final.log 2>&1 &
    PID=$!
    sleep 3
    
    if ps -p $PID > /dev/null; then
        echo "SUCCESS: Daemon estable en $PORT"
        ip link set emuccan0 txqueuelen 1000
        ip link set emuccan0 up
        ip link set emuccan1 txqueuelen 1000
        ip link set emuccan1 up
        echo "=== ESTADO FINAL ==="
        ip link show | grep emuccan
        exit 0
    else
        echo "FAILED: Daemon murió en $PORT. Log:"
        cat /tmp/emuc_final.log
    fi
done

echo "CRITICAL FAILURE: No se pudo iniciar en ningún puerto"
exit 1
EOF

chmod +x /tmp/restore_final.sh
echo 'FOX' | sudo -S /tmp/restore_final.sh
