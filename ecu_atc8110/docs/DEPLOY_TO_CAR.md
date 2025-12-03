# 🚗 Guía de Despliegue en el Coche - ECU ATC-8110

Esta guía te llevará paso a paso para desplegar el código de la ECU en el vehículo FOX de forma segura y eficiente.

---

## 📋 Pre-requisitos

### En tu PC de Desarrollo (Windows)

- [x] Código del proyecto en `C:\Users\ahech\Desktop\FOX`
- [x] Git instalado
- [x] Acceso SSH a la ECU configurado
- [x] GitHub Actions runner configurado (self-hosted)

### En la ECU del Coche (ATC-8110 - Ubuntu 18.04)

- [x] IP: `193.147.165.236`
- [x] Usuario: `fox`
- [x] SSH habilitado
- [x] Tarjeta CAN EMUC-B2S3 instalada
- [x] BMS conectado
- [x] Motores conectados

---

## 🔧 Configuración Inicial (Solo Primera Vez)

### 1. Configurar Acceso SSH sin Contraseña

Esto es **crítico** para el despliegue automático.

```powershell
# En tu PC Windows (PowerShell)

# Generar clave SSH si no la tienes
ssh-keygen -t ed25519 -C "tu_email@ejemplo.com"
# Presiona Enter para aceptar la ubicación por defecto
# Presiona Enter dos veces para no usar passphrase

# Copiar clave pública a la ECU
type $env:USERPROFILE\.ssh\id_ed25519.pub | ssh fox@193.147.165.236 "mkdir -p ~/.ssh && cat >> ~/.ssh/authorized_keys"

# Probar conexión (no debe pedir contraseña)
ssh fox@193.147.165.236 "echo 'Conexión exitosa'"
```

### 2. Preparar Directorio en la ECU

```bash
# Conectar a la ECU
ssh fox@193.147.165.236

# Crear estructura de directorios
mkdir -p /home/fox/ecu_app/{bin,logs,config}

# Crear script de reinicio
cat > /home/fox/ecu_app/restart.sh << 'EOF'
#!/bin/bash
# Script de reinicio de la ECU

# Detener proceso anterior si existe
pkill -9 ecu_atc8110 || true

# Esperar un momento
sleep 2

# Configurar interfaces CAN
sudo /home/fox/ecu_app/bin/setup_can.sh --real

# Iniciar nueva versión
cd /home/fox/ecu_app/bin
nohup sudo ./ecu_atc8110 > ../logs/ecu_$(date +%Y%m%d_%H%M%S).log 2>&1 &

echo "ECU reiniciada. PID: $!"
EOF

chmod +x /home/fox/ecu_app/restart.sh

# Dar permisos sudo sin contraseña para comandos específicos
echo "fox ALL=(ALL) NOPASSWD: /home/fox/ecu_app/bin/setup_can.sh" | sudo tee -a /etc/sudoers.d/ecu
echo "fox ALL=(ALL) NOPASSWD: /home/fox/ecu_app/bin/ecu_atc8110" | sudo tee -a /etc/sudoers.d/ecu
sudo chmod 0440 /etc/sudoers.d/ecu
```

---

## 🚀 Método 1: Despliegue Manual (Recomendado para Primera Vez)

### Paso 1: Compilar en la ECU

```bash
# Desde tu PC, transferir código
scp -r C:\Users\ahech\Desktop\FOX\ecu_atc8110 fox@193.147.165.236:/home/fox/

# Conectar a la ECU
ssh fox@193.147.165.236

# Compilar
cd /home/fox/ecu_atc8110
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# Verificar que compiló correctamente
ls -lh ecu_atc8110
```

**Salida esperada**:
```
-rwxr-xr-x 1 fox fox 2.5M Dec 01 09:45 ecu_atc8110
```

### Paso 2: Instalar en Directorio de Aplicación

```bash
# Copiar ejecutable
cp /home/fox/ecu_atc8110/build/ecu_atc8110 /home/fox/ecu_app/bin/

# Copiar script de configuración CAN
cp /home/fox/ecu_atc8110/scripts/setup_can.sh /home/fox/ecu_app/bin/
chmod +x /home/fox/ecu_app/bin/setup_can.sh
```

### Paso 3: Configurar Interfaces CAN

```bash
# Ejecutar script de configuración
sudo /home/fox/ecu_app/bin/setup_can.sh --real
```

**Verificar que las interfaces están UP**:
```bash
ip link show can0
ip link show can1
```

Deberías ver:
```
3: can0: <NOARP,UP,LOWER_UP,ECHO> mtu 16 qdisc pfifo_fast state UP ...
4: can1: <NOARP,UP,LOWER_UP,ECHO> mtu 16 qdisc pfifo_fast state UP ...
```

### Paso 4: Prueba Inicial (SIN arrancar el motor)

> [!CAUTION]
> **IMPORTANTE**: Esta prueba debe hacerse con el coche **APAGADO** y **SIN LLAVE DE CONTACTO**.

```bash
# Abrir 3 terminales SSH a la ECU

# Terminal 1: Monitorear CAN del BMS
candump can1

# Terminal 2: Monitorear CAN de motores
candump can0

# Terminal 3: Ejecutar ECU en modo debug
cd /home/fox/ecu_app/bin
sudo ./ecu_atc8110
```

**Verificaciones**:
- ✅ La ECU debe iniciar sin errores
- ✅ Debes ver mensajes de heartbeat en `can0` (ID `0x100`)
- ✅ Si el BMS está encendido, debes ver mensajes en `can1` (ID `0x180`)
- ✅ Los logs deben mostrar `[INFO] [SocketCAN] Interfaz can0 inicializada correctamente`

### Paso 5: Detener la Prueba

```bash
# En la Terminal 3, presionar Ctrl+C para detener la ECU
# Verificar que se detuvo correctamente
ps aux | grep ecu_atc8110
```

---

## 🤖 Método 2: Despliegue Automático con GitHub Actions

### Configurar Workflow

El workflow ya está configurado en `.github/workflows/deploy_to_ecu.yml`. Necesitas adaptarlo:

```yaml
name: CI-CD Windows -> ECU Linux

on:
  push:
    branches: [ "main" ]

jobs:
  build-and-deploy:
    runs-on: self-hosted
    steps:
      - name: Checkout código
        uses: actions/checkout@v4

      - name: Transferir a ECU
        run: |
          scp -r ecu_atc8110 fox@193.147.165.236:/home/fox/

      - name: Compilar en ECU
        run: |
          ssh fox@193.147.165.236 "cd /home/fox/ecu_atc8110 && mkdir -p build && cd build && cmake .. && make -j4"

      - name: Instalar en directorio de aplicación
        run: |
          ssh fox@193.147.165.236 "cp /home/fox/ecu_atc8110/build/ecu_atc8110 /home/fox/ecu_app/bin/ && cp /home/fox/ecu_atc8110/scripts/setup_can.sh /home/fox/ecu_app/bin/"

      - name: Reiniciar ECU
        run: |
          ssh fox@193.147.165.236 "/home/fox/ecu_app/restart.sh"

      - name: Verificar estado
        run: |
          sleep 5
          ssh fox@193.147.165.236 "ps aux | grep ecu_atc8110 | grep -v grep"
```

### Desplegar con Git

```powershell
# En tu PC Windows
cd C:\Users\ahech\Desktop\FOX

# Hacer cambios en el código...

# Commit y push
git add .
git commit -m "Actualización de código ECU"
git push origin main

# GitHub Actions se ejecutará automáticamente
# Monitorear en: https://github.com/tu-usuario/FOX/actions
```

---

## 🔍 Verificación Post-Despliegue

### Script de Verificación Automática

Crea este script en la ECU para verificar el estado:

```bash
# En la ECU
cat > /home/fox/ecu_app/check_status.sh << 'EOF'
#!/bin/bash

echo "========================================="
echo "  Estado de la ECU ATC-8110"
echo "========================================="
echo ""

# 1. Verificar proceso
echo "1. Proceso ECU:"
if pgrep -x ecu_atc8110 > /dev/null; then
    echo "   ✅ ECU corriendo (PID: $(pgrep -x ecu_atc8110))"
else
    echo "   ❌ ECU NO está corriendo"
fi
echo ""

# 2. Interfaces CAN
echo "2. Interfaces CAN:"
for iface in can0 can1; do
    if ip link show $iface &>/dev/null; then
        state=$(ip link show $iface | grep -oP 'state \K\w+')
        echo "   $iface: $state"
    else
        echo "   $iface: NO ENCONTRADA"
    fi
done
echo ""

# 3. Estadísticas CAN
echo "3. Mensajes CAN (últimos 5 segundos):"
timeout 5 candump can0,can1 2>/dev/null | wc -l | xargs echo "   Mensajes recibidos:"
echo ""

# 4. Últimos logs
echo "4. Últimos logs:"
tail -n 5 /home/fox/ecu_app/logs/ecu_*.log 2>/dev/null | tail -5
echo ""

echo "========================================="
EOF

chmod +x /home/fox/ecu_app/check_status.sh
```

**Ejecutar verificación**:
```bash
/home/fox/ecu_app/check_status.sh
```

---

## 🚨 Procedimiento de Seguridad en el Coche

### Antes de Arrancar el Motor

> [!WARNING]
> **PROTOCOLO DE SEGURIDAD OBLIGATORIO**

1. **Verificar conexiones físicas**:
   - [ ] Tarjeta EMUC-B2S3 bien conectada
   - [ ] Cables CAN bien conectados (CAN0 y CAN1)
   - [ ] BMS conectado y encendido
   - [ ] Controladores de motores conectados

2. **Verificar software**:
   ```bash
   # Ejecutar script de verificación
   /home/fox/ecu_app/check_status.sh
   ```

3. **Modo de prueba inicial**:
   - [ ] Coche en **PUNTO MUERTO**
   - [ ] Freno de mano **ACTIVADO**
   - [ ] Ruedas **BLOQUEADAS** (calzos)
   - [ ] Personal alejado del vehículo

### Secuencia de Arranque

```bash
# 1. Conectar a la ECU
ssh fox@193.147.165.236

# 2. Verificar estado
/home/fox/ecu_app/check_status.sh

# 3. Si todo está OK, arrancar
/home/fox/ecu_app/restart.sh

# 4. Monitorear en tiempo real
tail -f /home/fox/ecu_app/logs/ecu_*.log
```

### En Caso de Emergencia

```bash
# DETENER INMEDIATAMENTE
ssh fox@193.147.165.236 "sudo pkill -9 ecu_atc8110"

# Verificar que se detuvo
ssh fox@193.147.165.236 "ps aux | grep ecu_atc8110"
```

---

## 📊 Monitoreo Durante Operación

### Dashboard en Tiempo Real

Abre 4 terminales SSH:

**Terminal 1 - Logs de la ECU**:
```bash
ssh fox@193.147.165.236
tail -f /home/fox/ecu_app/logs/ecu_*.log
```

**Terminal 2 - CAN Motores**:
```bash
ssh fox@193.147.165.236
candump can0
```

**Terminal 3 - CAN BMS**:
```bash
ssh fox@193.147.165.236
candump can1
```

**Terminal 4 - Estadísticas**:
```bash
ssh fox@193.147.165.236
watch -n 1 'ip -s link show can0 && echo && ip -s link show can1'
```

---

## 🔄 Actualización del Software

### Actualización Rápida (Sin Recompilar)

Si solo cambias configuración:

```bash
# Detener ECU
ssh fox@193.147.165.236 "sudo pkill ecu_atc8110"

# Copiar nuevo archivo de configuración
scp config/nuevo_config.yaml fox@193.147.165.236:/home/fox/ecu_app/config/

# Reiniciar
ssh fox@193.147.165.236 "/home/fox/ecu_app/restart.sh"
```

### Actualización Completa (Con Recompilación)

```bash
# Método automático con Git
cd C:\Users\ahech\Desktop\FOX
git add .
git commit -m "Actualización v1.2"
git push origin main
# GitHub Actions desplegará automáticamente

# O método manual
scp -r ecu_atc8110 fox@193.147.165.236:/home/fox/
ssh fox@193.147.165.236 "cd /home/fox/ecu_atc8110/build && make && cp ecu_atc8110 /home/fox/ecu_app/bin/"
ssh fox@193.147.165.236 "/home/fox/ecu_app/restart.sh"
```

---

## 🐛 Solución de Problemas Comunes

### Problema: ECU no arranca

```bash
# Ver logs de error
ssh fox@193.147.165.236 "tail -50 /home/fox/ecu_app/logs/ecu_*.log"

# Verificar permisos
ssh fox@193.147.165.236 "ls -l /home/fox/ecu_app/bin/ecu_atc8110"

# Probar manualmente
ssh fox@193.147.165.236 "cd /home/fox/ecu_app/bin && sudo ./ecu_atc8110"
```

### Problema: Interfaces CAN no aparecen

```bash
# Verificar módulos del kernel
ssh fox@193.147.165.236 "lsmod | grep can"

# Recargar módulos
ssh fox@193.147.165.236 "sudo modprobe can && sudo modprobe can_raw"

# Ejecutar setup nuevamente
ssh fox@193.147.165.236 "sudo /home/fox/ecu_app/bin/setup_can.sh --real"
```

### Problema: No se reciben mensajes del BMS

```bash
# Verificar que can1 está UP
ssh fox@193.147.165.236 "ip link show can1"

# Verificar conexión física del BMS
# Verificar que el BMS está encendido

# Probar con candump
ssh fox@193.147.165.236 "timeout 10 candump can1"
```

---

## 📝 Checklist de Despliegue

Antes de cada despliegue en el coche:

- [ ] Código compilado sin errores en la ECU
- [ ] Tests pasados (si los hay)
- [ ] Backup del código anterior guardado
- [ ] Interfaces CAN configuradas y UP
- [ ] BMS respondiendo en can1
- [ ] Script de verificación ejecutado exitosamente
- [ ] Coche en condiciones de seguridad (freno, punto muerto)
- [ ] Personal informado del despliegue
- [ ] Procedimiento de emergencia revisado

---

## 📞 Contactos de Emergencia

- **Responsable técnico**: [Nombre]
- **Supervisor**: [Nombre]
- **Soporte remoto**: [Contacto]

---

## 📚 Recursos Adicionales

- [Documentación técnica completa](README.md)
- [Guía de deployment detallada](docs/DEPLOYMENT.md)
- [Protocolo CAN](docs/walkthrough.md#protocolo-can)
- [Solución de problemas](docs/DEPLOYMENT.md#solución-de-problemas)

---

**Última actualización**: 2025-12-01  
**Versión del documento**: 1.0

