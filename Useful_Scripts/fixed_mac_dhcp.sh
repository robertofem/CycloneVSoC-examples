#This file changes the MAC address of the eth0 
#interface and the calls udhcpc command to obtain
#an IP address. In the lab each MAC has its own IP
#assigned. Since DE1-SoC board generates a new one
#every time it boots, this script fixes a MAC in order
#DHCP running in router can assign a valid IP.
#To watch MAC before and after the change use 
#ip link show eth0

#before changing MAC bring interface down
ip link set dev eth0 down
#change MAC
ip link set dev eth0 address ba:11:24:7d:fd:75
#bring the interface up again 
ip link set dev eth0 up

#Now ask router a new IP
udhcpc



