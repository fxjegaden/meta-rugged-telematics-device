#!/bin/sh
modprobe brcmfmac
sleep 5
hostapd /etc/hostapd.conf -B
ifconfig wlan0 192.168.43.1
udhcpd
echo 1 > /proc/sys/net/ipv4/ip_forward
iptables -t nat -A POSTROUTING -o ppp0 -j MASQUERADE
iptables -A FORWARD -i ppp0 -o wlan0 -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -A FORWARD -i wlan0 -o ppp0 -j ACCEPT
iptables -t nat -S
