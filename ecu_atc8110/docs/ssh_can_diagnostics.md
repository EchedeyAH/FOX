# Guía Rápida: Diagnóstico CAN en la ECU

## Conexión SSH a la ECU

Desde tu PC Windows, conectar a la ECU por SSH:

```bash
# Reemplazar <IP_ECU> con la IP de tu ECU
ssh root@<IP_ECU>

# Ejemplo:
ssh root@192.168.1.100
```

## Paso 1: Configurar Interfaces CAN

Una vez conectado a la ECU por SSH:

```bash
cd /path/to/ecu_atc8110
sudo ./scripts/setup_can.sh --real
```

**Salida esperada**:
```
✓ emuccan0 activada (hardware EMUC-B2S3)
✓ emuccan1 activada (hardware EMUC-B2S3)
```

## Paso 2: Ejecutar Diagnóstico CAN

```bash
sudo ./scripts/diagnose_can.sh
```

Esto verificará:
- ✓ Interfaces CAN disponibles
- ✓ Tráfico RX/TX
- ✓ IDs CAN detectados (BMS, motores, supervisor)

## Paso 3: Monitor en Tiempo Real (Opcional)

Si compilaste el proyecto en la ECU:

```bash
./build/tools/can_diagnostic emuccan0
```

O usar `candump` directamente:

```bash
# Ver todos los mensajes CAN
candump emuccan0

# Ver solo mensajes del BMS (ID 0x180)
candump emuccan0,180:7FF

# Ver mensajes de motores (IDs 0x281-0x284)
candump emuccan0,281:7FC
```

## Comandos Útiles por SSH

```bash
# Ver interfaces CAN
ip link show | grep emuccan

# Ver estadísticas
ip -s link show emuccan0

# Enviar mensaje de prueba
cansend emuccan0 100#AABBCCDD

# Ver errores del sistema
dmesg | grep -i can | tail -20
```

## Ejemplo de Sesión SSH Completa

```bash
# 1. Conectar desde Windows
PS> ssh root@192.168.1.100

# 2. En la ECU
root@ecu:~# cd /root/ecu_atc8110
root@ecu:~/ecu_atc8110# sudo ./scripts/setup_can.sh --real
root@ecu:~/ecu_atc8110# sudo ./scripts/diagnose_can.sh

# 3. Si todo OK, ejecutar ECU principal
root@ecu:~/ecu_atc8110# ./build/ecu_atc8110

# 4. En otra terminal SSH (para monitorear)
PS> ssh root@192.168.1.100
root@ecu:~# candump emuccan0
```

## Solución de Problemas

### No puedo conectar por SSH

```bash
# Desde Windows, verificar conectividad
ping 192.168.1.100

# Verificar que SSH está corriendo en la ECU
# (necesitas acceso físico a la ECU)
systemctl status ssh
```

### "Permission denied" al ejecutar scripts

```bash
# Dar permisos de ejecución
chmod +x ./scripts/*.sh
chmod +x ./build/ecu_atc8110
```

### Scripts no se encuentran

```bash
# Verificar que estás en el directorio correcto
pwd
ls -la scripts/

# Navegar al directorio correcto
cd /root/ecu_atc8110
# o
cd /home/usuario/ecu_atc8110
```

## Transferir Archivos a la ECU (si es necesario)

```bash
# Desde Windows, copiar archivos a la ECU
scp C:\Users\ahech\Desktop\FOX\ecu_atc8110\scripts\diagnose_can.sh fox@193.147.165.236:/home/fox/ecu_atc8110/scripts/

# Copiar directorio completo
scp -r C:\Users\ahech\Desktop\FOX\ecu_atc8110\build fox@193.147.165.236:/home/fox/ecu_atc8110/
```

## IP de la ECU

Si no conoces la IP de la ECU:

1. **Acceso físico**: Conectar monitor y teclado, ejecutar `ip addr`
2. **Router**: Ver dispositivos conectados en el router
3. **Escaneo de red**: `nmap -sn 192.168.1.0/24` (desde Windows con nmap instalado)
