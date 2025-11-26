#!/usr/bin/env bash
set -euo pipefail

# Crea una interfaz vCAN para pruebas locales
sudo modprobe vcan || true
if ! ip link show vcan0 &>/dev/null; then
  sudo ip link add dev vcan0 type vcan
fi
sudo ip link set up vcan0

echo "Interfaz vcan0 lista para pruebas"
