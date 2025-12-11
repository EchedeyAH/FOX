#!/bin/bash
# Configuración CORRECTA para Innodisk en ttyACM1

# Crear script interno
cat > /tmp/config_acm1.sh << 'EOF'
#!/bin/bash
pkill -9 emucd_64
sleep 2

echo "Iniciando daemon en ttyACM1 (Innodisk)..."
# Probar con solo 2 interfaces primero, ya que B202 tiene 2 puertos
/usr/sbin/emucd_64 -s7 -e0 ttyACM1 emuccan0 emuccan1 > /tmp/emuc_acm1_v2.log 2>&1 &
PID=$!
sleep 3

if ps -p $PID > /dev/null; then
    echo "SUCCESS: Daemon corriendo en ttyACM1"
    ip link set emuccan0 txqueuelen 1000
    ip link set emuccan0 up
    ip link set emuccan1 txqueuelen 1000
    ip link set emuccan1 up
    echo "Interfaces configuradas:"
    ip link show | grep emuccan
else
    echo "FAILED: Daemon murió en ttyACM1"
    cat /tmp/emuc_acm1_v2.log
fi
EOF

chmod +x /tmp/config_acm1.sh
echo 'FOX' | sudo -S /tmp/config_acm1.sh
