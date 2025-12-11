#!/bin/bash
# Intentar iniciar en ttyACM2

# Crear script interno
cat > /tmp/try_acm2_root.sh << 'EOF'
#!/bin/bash
pkill -9 emucd_64
sleep 2

echo "Iniciando daemon en ttyACM2..."
/usr/sbin/emucd_64 -s7 -e0 ttyACM2 emuccan0 emuccan1 emuccan2 emuccan3 > /tmp/emuc_acm2.log 2>&1 &
PID=$!
sleep 3

if ps -p $PID > /dev/null; then
    echo "SUCCESS: Daemon corriendo en ttyACM2"
    ip link set emuccan0 txqueuelen 1000
    ip link set emuccan0 up
    ip link set emuccan1 txqueuelen 1000
    ip link set emuccan1 up
    echo "Interfaces configuradas"
else
    echo "FAILED: Daemon murió en ttyACM2"
    cat /tmp/emuc_acm2.log
fi
EOF

chmod +x /tmp/try_acm2_root.sh
echo 'FOX' | sudo -S /tmp/try_acm2_root.sh
