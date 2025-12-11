#!/bin/bash
# Intentar iniciar en ttyACM1

# Crear script interno
cat > /tmp/try_acm1_root.sh << 'EOF'
#!/bin/bash
pkill -9 emucd_64
sleep 2

echo "Iniciando daemon en ttyACM1..."
/usr/sbin/emucd_64 -s7 -e0 ttyACM1 emuccan0 emuccan1 emuccan2 emuccan3 > /tmp/emuc_acm1.log 2>&1 &
PID=$!
sleep 3

if ps -p $PID > /dev/null; then
    echo "SUCCESS: Daemon corriendo en ttyACM1"
    ip link set emuccan0 txqueuelen 1000
    ip link set emuccan0 up
    ip link set emuccan1 txqueuelen 1000
    ip link set emuccan1 up
    echo "Interfaces configuradas"
else
    echo "FAILED: Daemon murió en ttyACM1"
    cat /tmp/emuc_acm1.log
fi
EOF

chmod +x /tmp/try_acm1_root.sh
echo 'FOX' | sudo -S /tmp/try_acm1_root.sh
