# Configuración Permanente de Interfaces CAN

Este documento explica cómo configurar las interfaces CAN para que se inicialicen automáticamente al arrancar la ECU.

## Archivos Creados

1. **`can_startup.sh`**: Script que detecta el puerto USB y configura las interfaces CAN
2. **`fox-can.service`**: Servicio systemd para ejecución automática
3. **`install_can_service.sh`**: Script de instalación del servicio

## Instalación

### Paso 1: Subir archivos a la ECU

Desde tu PC Windows, copia los archivos al directorio de scripts:

```powershell
scp ecu_atc8110/scripts/can_startup.sh fox@193.147.165.236:/home/fox/ecu_atc8110/scripts/
scp ecu_atc8110/scripts/fox-can.service fox@193.147.165.236:/home/fox/ecu_atc8110/scripts/
scp ecu_atc8110/scripts/install_can_service.sh fox@193.147.165.236:/home/fox/ecu_atc8110/scripts/
```

### Paso 2: Instalar el servicio

Conéctate a la ECU vía SSH y ejecuta:

```bash
cd ~/ecu_atc8110/scripts
chmod +x install_can_service.sh
sudo ./install_can_service.sh
```

El script de instalación hará lo siguiente:
- Configurar permisos de ejecución
- Copiar el servicio a `/etc/systemd/system/`
- Habilitar el servicio para inicio automático
- Iniciar el servicio inmediatamente

### Paso 3: Verificar

Verifica que el servicio esté funcionando:

```bash
# Ver estado del servicio
sudo systemctl status fox-can

# Ver logs en tiempo real
sudo journalctl -u fox-can -f

# Verificar interfaces CAN
ip link show emuccan0
ip link show emuccan1
```

## Configuración

El script `can_startup.sh` está configurado para:

- **emuccan0**: Motores + Supervisor (1 Mbps, código `-s8`)
- **emuccan1**: BMS (500 Kbps, código `-s7` si el driver lo soporta)

### Ajustar Velocidades

Si necesitas cambiar las velocidades CAN, edita el archivo `can_startup.sh` en la línea:

```bash
/usr/sbin/emucd_64 -s8 -e0 "$EMUC_PORT" emuccan0 emuccan1 emuccan2 emuccan3
```

Códigos comunes:
- `-s8`: 1 Mbps
- `-s7`: 500 Kbps
- `-s6`: 250 Kbps

> **Nota**: Algunos drivers EMUC solo aceptan un código `-s` para todos los puertos. Si el BMS requiere 500 Kbps y los motores 1 Mbps, puede que necesites usar dos instancias del daemon en diferentes puertos USB.

## Comandos Útiles

```bash
# Ver estado del servicio
sudo systemctl status fox-can

# Ver logs
sudo journalctl -u fox-can -f

# Reiniciar servicio
sudo systemctl restart fox-can

# Detener servicio
sudo systemctl stop fox-can

# Deshabilitar inicio automático
sudo systemctl disable fox-can

# Habilitar inicio automático
sudo systemctl enable fox-can
```

## Logs

El servicio guarda logs en dos lugares:

1. **Systemd journal**: `sudo journalctl -u fox-can`
2. **Archivo de log**: `/var/log/fox_can_startup.log`

## Solución de Problemas

### El servicio no inicia

```bash
# Ver logs detallados
sudo journalctl -u fox-can -n 50

# Verificar que existe el puerto USB
ls -la /dev/ttyACM*

# Probar el script manualmente
sudo /home/fox/ecu_atc8110/scripts/can_startup.sh
```

### Las interfaces no aparecen

```bash
# Verificar que el daemon está corriendo
ps aux | grep emucd_64

# Ver log del daemon
cat /tmp/emuc_daemon.log

# Verificar módulos del kernel
lsmod | grep can
```

### Cambiar puerto USB

El script detecta automáticamente el puerto (`ttyACM0`, `ttyACM1`, etc.). Si necesitas forzar un puerto específico, edita `can_startup.sh` y cambia la sección de detección de puerto.

## Desinstalación

Para eliminar el servicio:

```bash
sudo systemctl stop fox-can
sudo systemctl disable fox-can
sudo rm /etc/systemd/system/fox-can.service
sudo systemctl daemon-reload
```
