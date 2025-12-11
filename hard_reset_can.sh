#!/bin/bash
# Script de reinicio PROFUNDO para CAN

# 1. Crear script interno
cat > /tmp/reset_can_root.sh << 'EOF'
#!/bin/bash
echo "=== 1. Deteniendo servicios ==="
pkill -9 emucd_64
sleep 1

echo "=== 2. Recargando módulos del kernel ==="
modprobe -r emuc2socketcan
sleep 1
modprobe emuc2socketcan
sleep 1
dmesg | tail -5

echo "=== 3. Iniciando daemon EMUC ==="
# Intentar iniciar
/usr/sbin/emucd_64 -s7 -e0 ttyACM0 emuccan0 emuccan1 emuccan2 emuccan3 > /tmp/emuc_start.log 2>&1 &
PID=$!
echo "Daemon lanzado con PID $PID"
sleep 4

# Verificar
if ps -p $PID > /dev/null; then
    echo "=== 4. Daemon OK. Levantando interfaces ==="
    ip link set emuccan0 txqueuelen 1000
    ip link set emuccan0 up
    ip link set emuccan1 txqueuelen 1000
    ip link set emuccan1 up
    echo "Interfaces configuradas"
else
    echo "CRITICAL: Daemon murió. Salida log:"
    cat /tmp/emuc_start.log
    
    echo "=== Intento alternativo sin daemon (si el módulo crea interfaces solo) ==="
    ip link show
fi
EOF

chmod +x /tmp/reset_can_root.sh

# ejecutar
echo 'FOX' | sudo -S /tmp/reset_can_root.sh
