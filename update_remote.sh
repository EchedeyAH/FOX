#!/bin/bash
set -e

echo "Starting ECU update..."

# 1. Log directory
echo "Updating log directory..."
if [ -d "/var/log/ecu_atx1610" ]; then
    mv /var/log/ecu_atx1610 /var/log/ecu_atc8110
elif [ ! -d "/var/log/ecu_atc8110" ]; then
    mkdir -p /var/log/ecu_atc8110
fi
chown -R fox:fox /var/log/ecu_atc8110

# 2. Sudoers
echo "Updating sudoers..."
if ! grep -q "ecu_atc8110" /etc/sudoers.d/ecu; then
    echo "fox ALL=(ALL) NOPASSWD: /home/fox/ecu_app/bin/ecu_atc8110" >> /etc/sudoers.d/ecu
fi

# 3. Systemd service
echo "Updating systemd service..."
if [ -f /etc/systemd/system/ecu-atx1610.service ]; then
    echo "Stopping old service..."
    systemctl stop ecu-atx1610 || true
    systemctl disable ecu-atx1610 || true
    
    echo "Renaming service file..."
    mv /etc/systemd/system/ecu-atx1610.service /etc/systemd/system/ecu-atc8110.service
    
    echo "Updating service content..."
    sed -i "s/ecu_atx1610/ecu_atc8110/g" /etc/systemd/system/ecu-atc8110.service
    sed -i "s/ATX1610/ATC8110/g" /etc/systemd/system/ecu-atc8110.service
    
    echo "Reloading systemd..."
    systemctl daemon-reload
    systemctl enable ecu-atc8110
    systemctl start ecu-atc8110
    echo "Service updated and started."
else
    echo "Old service not found. Checking for new service..."
    if [ -f /etc/systemd/system/ecu-atc8110.service ]; then
        echo "New service already exists. Restarting..."
        systemctl restart ecu-atc8110
    else
        echo "No service found to update."
    fi
fi

# 4. Update helper scripts
echo "Updating helper scripts..."
cp /home/fox/restart.sh /home/fox/ecu_app/restart.sh
cp /home/fox/check_status.sh /home/fox/ecu_app/check_status.sh
chmod +x /home/fox/ecu_app/restart.sh
chmod +x /home/fox/ecu_app/check_status.sh

echo "Update complete!"
