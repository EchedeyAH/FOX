### parameter
socket_name_1=emuccan0
socket_name_2=emuccan1
dev_name=ttyACM0
baudrate=7  # 0~3: support FW version >= 03.00
            # 0: 5   KBPS, 1: 10  KBPS,  2: 20  KBPS, 3: 50  KBPS,
            # 4: 100 KBPS, 5: 125 KBPS,  6: 250 KBPS, 7: 500 KBPS,
            # 8: 800 KBPS, 9: 1 MBPS,   10: 400 KBPS
error_type=0 # 0: EMUC_DIS_ALL, 1: EMUC_EE_ERR, 2: EMUC_BUS_ERR, 3: EMUC_EN_ALL

### step
sudo pkill -2 emucd_64
sleep 0.2
sudo rmmod emuc2socketcan
# sudo insmod emuc2socketcan.ko
sudo cp emuc2socketcan.ko /lib/modules/$(uname -r)/kernel/drivers/net/can
sudo depmod -a
sudo modprobe emuc2socketcan
sudo ./emucd_64 -s${baudrate} -e${error_type} ${dev_name} ${socket_name_1} ${socket_name_2}

if command -v ifconfig &> /dev/null
then
    sudo ifconfig ${socket_name_1} txqueuelen 1000
    sudo ifconfig ${socket_name_2} txqueuelen 1000
    sudo tc qdisc add dev ${socket_name_1} root handle 1: pfifo
    sudo tc qdisc add dev ${socket_name_2} root handle 1: pfifo
    sudo ifconfig ${socket_name_1} up
    sudo ifconfig ${socket_name_2} up
elif command -v ip &> /dev/null
then
    sudo ip link set ${socket_name_1} txqueuelen 1000
    sudo ip link set ${socket_name_2} txqueuelen 1000
    sudo tc qdisc add dev ${socket_name_1} root handle 1: pfifo
    sudo tc qdisc add dev ${socket_name_2} root handle 1: pfifo
    sudo ip link set ${socket_name_1} up
    sudo ip link set ${socket_name_2} up
else
    echo "ip & ifconfig command could not be found"
fi
