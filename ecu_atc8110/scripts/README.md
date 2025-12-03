# Scripts de Utilidad para la ECU

Este directorio contiene scripts auxiliares para facilitar el despliegue y operación de la ECU.

## 📜 Scripts Disponibles

### `setup_can.sh`
Script principal para configurar las interfaces CAN.

**Uso**:
```bash
# Modo real (hardware EMUC-B2S3)
sudo ./setup_can.sh --real

# Modo virtual (testing sin hardware)
sudo ./setup_can.sh --virtual

# Ayuda
./setup_can.sh --help
```

**Funcionalidades**:
- Carga módulos del kernel CAN
- Configura bitrates (can0 @ 1 Mbps, can1 @ 500 Kbps)
- Verifica estado de interfaces
- Soporta modo virtual para desarrollo

---

## 🔧 Scripts Adicionales Recomendados

### `restart.sh` (Crear en la ECU)

Script para reiniciar la ECU de forma segura.

```bash
#!/bin/bash
# Ubicación: /home/fox/ecu_app/restart.sh

# Detener proceso anterior
pkill -9 ecu_atc8110 || true
sleep 2

# Configurar CAN
sudo /home/fox/ecu_app/bin/setup_can.sh --real

# Iniciar ECU
cd /home/fox/ecu_app/bin
nohup sudo ./ecu_atc8110 > ../logs/ecu_$(date +%Y%m%d_%H%M%S).log 2>&1 &

echo "ECU reiniciada. PID: $!"
```

### `check_status.sh` (Crear en la ECU)

Script para verificar el estado del sistema.

```bash
#!/bin/bash
# Ubicación: /home/fox/ecu_app/check_status.sh

echo "========================================="
echo "  Estado de la ECU ATX-1610"
echo "========================================="

# Verificar proceso
if pgrep -x ecu_atc8110 > /dev/null; then
    echo "✅ ECU corriendo (PID: $(pgrep -x ecu_atc8110))"
else
    echo "❌ ECU NO está corriendo"
fi

# Verificar interfaces CAN
for iface in can0 can1; do
    if ip link show $iface &>/dev/null; then
        state=$(ip link show $iface | grep -oP 'state \K\w+')
        echo "$iface: $state"
    fi
done

# Mostrar últimos logs
echo ""
echo "Últimos logs:"
tail -5 /home/fox/ecu_app/logs/ecu_*.log 2>/dev/null
```

### `monitor_can.sh` (Crear en la ECU)

Script para monitorear tráfico CAN en tiempo real.

```bash
#!/bin/bash
# Ubicación: /home/fox/ecu_app/monitor_can.sh

echo "Monitoreando tráfico CAN (Ctrl+C para detener)..."
echo "CAN0 (Motores) | CAN1 (BMS)"
echo "========================================="

# Monitorear ambas interfaces simultáneamente
candump can0,can1 -c -t A
```

### `backup_logs.sh` (Crear en la ECU)

Script para hacer backup de logs.

```bash
#!/bin/bash
# Ubicación: /home/fox/ecu_app/backup_logs.sh

BACKUP_DIR="/home/fox/ecu_app/logs/backup"
mkdir -p $BACKUP_DIR

# Crear archivo tar con fecha
tar -czf $BACKUP_DIR/logs_$(date +%Y%m%d_%H%M%S).tar.gz \
    /home/fox/ecu_app/logs/ecu_*.log

# Limpiar logs antiguos (mantener últimos 10)
cd $BACKUP_DIR
ls -t logs_*.tar.gz | tail -n +11 | xargs rm -f

echo "Backup completado en $BACKUP_DIR"
```

---

## 📋 Instalación de Scripts en la ECU

```bash
# Conectar a la ECU
ssh fox@193.147.165.236

# Crear directorio de scripts si no existe
mkdir -p /home/fox/ecu_app/bin

# Los scripts se pueden crear con los comandos mostrados arriba
# o transferirlos desde tu PC:

# Desde tu PC Windows (PowerShell):
# scp scripts/*.sh fox@193.147.165.236:/home/fox/ecu_app/bin/

# Dar permisos de ejecución
chmod +x /home/fox/ecu_app/bin/*.sh
```

---

## 🔐 Configuración de Permisos Sudo

Para que los scripts funcionen sin pedir contraseña:

```bash
# En la ECU
sudo visudo -f /etc/sudoers.d/ecu

# Añadir estas líneas:
fox ALL=(ALL) NOPASSWD: /home/fox/ecu_app/bin/setup_can.sh
fox ALL=(ALL) NOPASSWD: /home/fox/ecu_app/bin/ecu_atc8110
fox ALL=(ALL) NOPASSWD: /sbin/ip
fox ALL=(ALL) NOPASSWD: /sbin/modprobe

# Guardar y salir
# Verificar permisos:
sudo chmod 0440 /etc/sudoers.d/ecu
```

---

## 📚 Uso Común

### Despliegue Completo

```bash
# 1. Transferir código
scp -r ecu_atc8110 fox@193.147.165.236:/home/fox/

# 2. Compilar
ssh fox@193.147.165.236 "cd /home/fox/ecu_atc8110/build && cmake .. && make"

# 3. Instalar
ssh fox@193.147.165.236 "cp /home/fox/ecu_atc8110/build/ecu_atc8110 /home/fox/ecu_app/bin/"

# 4. Reiniciar
ssh fox@193.147.165.236 "/home/fox/ecu_app/restart.sh"

# 5. Verificar
ssh fox@193.147.165.236 "/home/fox/ecu_app/check_status.sh"
```

### Monitoreo

```bash
# Ver logs en tiempo real
ssh fox@193.147.165.236 "tail -f /home/fox/ecu_app/logs/ecu_*.log"

# Monitorear CAN
ssh fox@193.147.165.236 "/home/fox/ecu_app/monitor_can.sh"

# Verificar estado
ssh fox@193.147.165.236 "/home/fox/ecu_app/check_status.sh"
```

---

## 🔗 Referencias

- [Guía de despliegue en el coche](../docs/DEPLOY_TO_CAR.md)
- [Documentación de SocketCAN](https://www.kernel.org/doc/html/latest/networking/can.html)
- [Manual de can-utils](https://github.com/linux-can/can-utils)
