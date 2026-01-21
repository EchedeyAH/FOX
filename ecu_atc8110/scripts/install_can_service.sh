#!/bin/bash
#
# Script de instalación del servicio systemd para CAN
#

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=========================================${NC}"
echo -e "${GREEN}  Instalación de Servicio CAN${NC}"
echo -e "${GREEN}=========================================${NC}"
echo ""

# Verificar permisos de root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}Error: Este script requiere permisos de root${NC}"
    echo "Ejecutar con: sudo $0"
    exit 1
fi

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
STARTUP_SCRIPT="$SCRIPT_DIR/can_startup.sh"
SERVICE_FILE="$SCRIPT_DIR/fox-can.service"

# Verificar que existen los archivos
if [ ! -f "$STARTUP_SCRIPT" ]; then
    echo -e "${RED}Error: No se encuentra can_startup.sh${NC}"
    exit 1
fi

if [ ! -f "$SERVICE_FILE" ]; then
    echo -e "${RED}Error: No se encuentra fox-can.service${NC}"
    exit 1
fi

# Hacer ejecutable el script de inicio
echo -e "${YELLOW}[1/5] Configurando permisos...${NC}"
chmod +x "$STARTUP_SCRIPT"
echo "✓ Script de inicio configurado"

# Copiar el servicio a systemd
echo -e "${YELLOW}[2/5] Instalando servicio systemd...${NC}"
cp "$SERVICE_FILE" /etc/systemd/system/fox-can.service
echo "✓ Servicio copiado a /etc/systemd/system/"

# Recargar systemd
echo -e "${YELLOW}[3/5] Recargando systemd...${NC}"
systemctl daemon-reload
echo "✓ Systemd recargado"

# Habilitar el servicio para que arranque automáticamente
echo -e "${YELLOW}[4/5] Habilitando servicio...${NC}"
systemctl enable fox-can.service
echo "✓ Servicio habilitado para inicio automático"

# Iniciar el servicio ahora
echo -e "${YELLOW}[5/5] Iniciando servicio...${NC}"
systemctl start fox-can.service
echo "✓ Servicio iniciado"

echo ""
echo -e "${GREEN}=========================================${NC}"
echo -e "${GREEN}  Instalación completada${NC}"
echo -e "${GREEN}=========================================${NC}"
echo ""
echo "El servicio CAN se iniciará automáticamente en cada arranque."
echo ""
echo "Comandos útiles:"
echo "  - Ver estado:    sudo systemctl status fox-can"
echo "  - Ver logs:      sudo journalctl -u fox-can -f"
echo "  - Reiniciar:     sudo systemctl restart fox-can"
echo "  - Deshabilitar:  sudo systemctl disable fox-can"
echo ""

# Mostrar estado actual
echo -e "${YELLOW}Estado actual del servicio:${NC}"
systemctl status fox-can --no-pager || true

exit 0
