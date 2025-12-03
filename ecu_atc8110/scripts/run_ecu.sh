#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."

if [ ! -f "${ROOT_DIR}/build/ecu_atx1610" ]; then
  echo "[run_ecu] Ejecutable no encontrado. Compile con: mkdir -p build && cd build && cmake .. && cmake --build ."
  exit 1
fi

cd "${ROOT_DIR}/build"
./ecu_atx1610
