#!/bin/bash
# verify_can.sh - Bring up CAN interfaces and display status
# Usage: ./verify_can.sh

set -e

# Bring up interfaces (if not already up)
echo 'FOX' | sudo -S ip link set emuccan0 up || true
echo 'FOX' | sudo -S ip link set emuccan1 up || true

# Show interface status
echo "=== Interface Status ==="
ip -details link show emuccan0
ip -details link show emuccan1

echo "=== CAN Traffic (5 seconds) ==="
# If candump is available, capture a few frames
if command -v candump >/dev/null 2>&1; then
  echo 'FOX' | sudo -S candump -i emuccan0 -t a -n 10
else
  echo "candump not installed; skipping traffic capture."
fi
