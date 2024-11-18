### parameter
socket_name_1=emuccan0
socket_name_2=emuccan1

### step
sudo ip link set ${socket_name_1} down
sudo ip link set ${socket_name_2} down
sudo tc qdisc del dev ${socket_name_1} root
sudo tc qdisc del dev ${socket_name_2} root
sudo pkill -2 emucd_64
sleep 0.2
sudo rmmod emuc2socketcan
#rm /lib/modules/$(uname -r)/kernel/drivers/net/can/emuc2socketcan.ko
