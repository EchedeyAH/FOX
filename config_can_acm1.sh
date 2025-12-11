#!/bin/bash
# Configuración para ttyACM1

cat > /tmp/config_acm1_final.sh << 'EOF'
#!/bin/bash
pkill -9 emucd_64
sleep 2

echo "Iniciando daemon en ttyACM1..."
# Intentar con 4 interfaces pero usando ttyACM1
/usr/sbin/emucd_64 -s7 -e0 ttyACM1 emuccan0 emuccan1 emuccan2 emuccan3 > /tmp/emuc_acm1_final.log 2>&1 &
PID=$!
sleep 3

if ps -p $PID > /dev/null; then
    echo "SUCCESS: Daemon corriendo en ttyACM1 (PID $PID)"
    ip link set emuccan0 txqueuelen 1000
    ip link set emuccan0 up
    ip link set emuccan1 txqueuelen 1000
    ip link set emuccan1 up
    echo "Interfaces:"
    ip link show | grep emuccan
else
    echo "FAILED: Daemon murió en ttyACM1"
    cat /tmp/emuc_acm1_final.log
fi
EOF

chmod +x /tmp/config_acm1_final.sh
echo 'FOX' | sudo -S /tmp/config_acm1_final.sh
