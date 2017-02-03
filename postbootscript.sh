#!/bin/bash
apt-get update 
apt-get -y install iperf 
apt-get -y install tshark git gcc g++
git clone https://github.com/rahul-daga-imgtec/QOSPF.git 
echo '100  QoSRT' >> /etc/iproute2/rt_tables
sysctl net.ipv4.ip_forward=1
ip rule add tos 0x04 table QoSRT
cd QOSPF
getent hosts 127.255.255.1 | tail -c 2 > node_id.txt
g++ Dijkstra.cpp -o Dijkstra.o 
gcc congestion_reporter.c -o congestion.o