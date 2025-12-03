# Guía de Despliegue - ECU ATX-1610

## Requisitos Previos

### Hardware Necesario
- ✅ ECU ATC-8110 con Ubuntu 18.04 LTS
- ✅ Tarjeta CAN EMUC-B2S3 instalada
- ✅ BMS conectado al bus CAN1 (500 Kbps)
- ✅ Controladores de motores conectados al bus CAN0 (1 Mbps)
- ✅ Sensores analógicos (PEX-1202L)
- ✅ Salidas digitales/analógicas (PEX-DA16)
- ✅ GPS ublox SM-76G

### Software en la ECU
```bash
# Verificar versión de Ubuntu
lsb_release -a
# Debe ser: Ubuntu 18.04 LTS

# Instalar dependencias
sudo apt-get update
sudo apt-get install -y build-essential cmake git can-utils
```

---

## Paso 1: Transferir Código a la ECU

### Opción A: Desde repositorio Git
```bash
# En la ECU
cd /home/fox
git clone <URL_DEL_REPOSITORIO> FOX
cd FOX/ecu_atc8110
```

### Opción B: Transferencia directa vía SSH
```bash
# Desde tu PC Windows (PowerShell)
scp -r C:\Users\ahech\Desktop\FOX\ecu_atc8110 fox@193.147.165.236:/home/fox/

# Conectar a la ECU
ssh fox@193.147.165.236
cd /home/fox/ecu_atc8110
```

---

## Paso 2: Compilar el Proyecto

```bash
# Crear directorio de build
mkdir -p build
cd build

# Configurar con CMake
cmake ..

# Compilar
make -j$(nproc)

# Verificar que se generó el ejecutable
ls -lh ecu_atc8110
```

**Salida esperada**:
```
-rwxr-xr-x 1 fox fox 2.5M Nov 27 15:30 ecu_atc8110
```

---

## Paso 3: Configurar Interfaces CAN

```bash
# Volver al directorio raíz del proyecto
cd /home/fox/ecu_atc8110

# Dar permisos de ejecución al script
chmod +x scripts/setup_can.sh

# Ejecutar configuración de hardware real
sudo ./scripts/setup_can.sh --real
```

**Salida esperada**:
```
=========================================
  Configuración de Interfaces CAN
  ECU ATX-1610
=========================================

Modo seleccionado: real

[1/4] Cargando módulos del kernel...
✓ Módulos CAN cargados

[2/4] Configurando interfaces CAN...
✓ can0 configurada @ 1 Mbps y activada
✓ can1 configurada @ 500 Kbps y activada

[3/4] Verificando estado de interfaces...
  can0: UP
  can1: UP

[4/4] Configuración completada
✓ Configuración completada exitosamente
```

### Verificar Interfaces CAN

```bash
# Ver estado de las interfaces
ip link show can0
ip link show can1

# Ver estadísticas
ip -s link show can0
ip -s link show can1
```

---

## Paso 4: Pruebas Previas al Arranque

### Prueba 1: Verificar Comunicación con BMS

```bash
# En una terminal, monitorear mensajes CAN del BMS
candump can1

# Deberías ver mensajes con ID 0x180 si el BMS está conectado
```

**Ejemplo de salida**:
```
can1  180   [8]  30 31 56 30 44 38 38 00
can1  180   [8]  30 31 54 30 32 35 00 00
```

### Prueba 2: Verificar Comunicación con Motores

```bash
# Monitorear mensajes de motores
candump can0

# Deberías ver respuestas de los controladores (IDs 0x281-0x284)
```

---

## Paso 5: Ejecutar la ECU

### Ejecución Normal

```bash
cd /home/fox/ecu_atc8110/build

# Ejecutar con permisos de root (necesario para CAN)
sudo ./ecu_atc8110
```

**Salida esperada**:
```
[INFO] [SocketCAN] Interfaz can0 inicializada correctamente
[INFO] [SensorManager] Sensores inicializados
[INFO] [FSM] Transición a estado 1
ECU ATX1610 skeleton running
[INFO] [Supervisor] Iteración 0
[DEBUG] [SocketCAN] TX id=0x100 dlc=3
[DEBUG] [SocketCAN] TX id=0x180 dlc=3
[INFO] [Supervisor] Iteración 1
...
```

### Ejecución en Background

```bash
# Ejecutar como servicio en background
sudo nohup ./ecu_atc8110 > /var/log/ecu_atc8110.log 2>&1 &

# Ver el PID del proceso
echo $!

# Ver logs en tiempo real
tail -f /var/log/ecu_atc8110.log
```

### Detener la ECU

```bash
# Encontrar el proceso
ps aux | grep ecu_atc8110

# Detener (reemplazar <PID> con el número del proceso)
sudo kill <PID>

# O detener todos los procesos ecu_atc8110
sudo pkill ecu_atc8110
```

---

## Paso 6: Monitoreo en Tiempo Real

### Terminal 1: Ejecutar ECU
```bash
sudo ./ecu_atc8110
```

### Terminal 2: Monitorear CAN0 (Motores)
```bash
candump can0
```

### Terminal 3: Monitorear CAN1 (BMS)
```bash
candump can1
```

### Terminal 4: Ver Estadísticas
```bash
watch -n 1 'ip -s link show can0 && echo && ip -s link show can1'
```

---

## Configuración como Servicio Systemd

Para que la ECU arranque automáticamente al iniciar el sistema:

### 1. Crear archivo de servicio

```bash
sudo nano /etc/systemd/system/ecu-atx1610.service
```

**Contenido**:
```ini
[Unit]
Description=ECU ATX-1610 Control System
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=/home/fox/ecu_atc8110/build
ExecStartPre=/home/fox/ecu_atc8110/scripts/setup_can.sh --real
ExecStart=/home/fox/ecu_atc8110/build/ecu_atc8110
Restart=on-failure
RestartSec=5s
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

### 2. Habilitar y arrancar el servicio

```bash
# Recargar configuración de systemd
sudo systemctl daemon-reload

# Habilitar inicio automático
sudo systemctl enable ecu-atx1610

# Iniciar el servicio
sudo systemctl start ecu-atx1610

# Ver estado
sudo systemctl status ecu-atx1610

# Ver logs
sudo journalctl -u ecu-atx1610 -f
```

---

## Solución de Problemas

### Problema: Interfaces CAN no aparecen

**Síntoma**: `ip link show can0` no muestra la interfaz

**Solución**:
```bash
# Verificar que la tarjeta EMUC-B2S3 está instalada
lspci | grep -i can

# Cargar módulos manualmente
sudo modprobe can
sudo modprobe can_raw
sudo modprobe sja1000
sudo modprobe sja1000_platform

# Verificar módulos cargados
lsmod | grep can
```

### Problema: Error "Permission denied" al abrir socket CAN

**Síntoma**: `Error creando socket CAN: Permission denied`

**Solución**:
```bash
# Ejecutar con sudo
sudo ./ecu_atc8110

# O añadir usuario al grupo can (requiere reinicio)
sudo usermod -a -G can fox
```

### Problema: No se reciben mensajes del BMS

**Síntoma**: `candump can1` no muestra mensajes

**Solución**:
```bash
# Verificar que can1 está UP
ip link show can1

# Verificar conexión física del BMS
# Verificar que el BMS está encendido
# Verificar bitrate correcto (500 Kbps)
sudo ip link set can1 type can bitrate 500000
sudo ip link set can1 up
```

### Problema: Bus-off error

**Síntoma**: Interfaz CAN entra en estado BUS-OFF

**Solución**:
```bash
# Reiniciar interfaz
sudo ip link set can0 down
sudo ip link set can0 type can bitrate 1000000
sudo ip link set can0 up

# Verificar errores
ip -s link show can0
```

---

## Verificación Post-Despliegue

### Checklist de Verificación

- [ ] Código compilado sin errores
- [ ] Interfaces CAN configuradas (can0 @ 1 Mbps, can1 @ 500 Kbps)
- [ ] BMS enviando mensajes en can1 (ID 0x180)
- [ ] Motores respondiendo en can0 (IDs 0x281-0x284)
- [ ] ECU enviando heartbeat (ID 0x100)
- [ ] Datos de batería actualizándose correctamente
- [ ] Sin errores en logs
- [ ] Estadísticas CAN sin errores de TX/RX

### Comandos de Verificación Rápida

```bash
# Script de verificación completa
cat << 'EOF' > /tmp/verify_ecu.sh
#!/bin/bash
echo "=== Verificación ECU ATX-1610 ==="
echo ""
echo "1. Interfaces CAN:"
ip link show can0 | grep -E "can0|state"
ip link show can1 | grep -E "can1|state"
echo ""
echo "2. Proceso ECU:"
ps aux | grep ecu_atc8110 | grep -v grep
echo ""
echo "3. Estadísticas CAN0:"
ip -s link show can0 | grep -A 2 "RX:"
echo ""
echo "4. Estadísticas CAN1:"
ip -s link show can1 | grep -A 2 "RX:"
echo ""
echo "5. Últimos logs:"
sudo journalctl -u ecu-atx1610 -n 10 --no-pager
EOF

chmod +x /tmp/verify_ecu.sh
/tmp/verify_ecu.sh
```

---

## Actualización del Software

Para actualizar el código después del despliegue inicial:

```bash
# Detener servicio si está corriendo
sudo systemctl stop ecu-atx1610

# Actualizar código
cd /home/fox/ecu_atc8110
git pull  # Si usas Git

# Recompilar
cd build
make clean
cmake ..
make -j$(nproc)

# Reiniciar servicio
sudo systemctl start ecu-atx1610
```

---

## Contacto y Soporte

Para problemas o preguntas sobre el despliegue, consultar:
- [Documentación técnica](README.md)
- [Walkthrough de implementación](walkthrough.md)
- Logs del sistema: `/var/log/ecu_atc8110.log`
- Logs de systemd: `sudo journalctl -u ecu-atx1610`
