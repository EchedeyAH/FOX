sudo rm -f /usr/sbin/emucd_64
sudo rm -f /etc/init.d/run_emucd
# sudo rm -f /etc/init.d/emuc2socketcan.ko
sudo rm -f /lib/modules/$(uname -r)/kernel/drivers/net/can/emuc2socketcan.ko
sudo cp ../emucd_64 /usr/sbin
sudo cp ./run_emucd /etc/init.d
# sudo cp ../emuc2socketcan.ko /etc/init.d
sudo cp ../emuc2socketcan.ko /lib/modules/$(uname -r)/kernel/drivers/net/can
sudo depmod -a
sudo chmod +x /etc/init.d/run_emucd
sudo update-rc.d run_emucd defaults
