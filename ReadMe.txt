This is a readme file for the ECE-573 Class Project "Improving latency and reliability for high priority data in a mesh network using QOSPF - a QoS extension to OSPF protocol".
Submitted by:
Rahul Daga
Lahari Kommi
Bikram Singh
Pranjali CHumbhale.

Setup Instructions:

1. Create the demo topology in exogeni.

2. Include the postbootscript.sh as a postbootscript for all nodes in exogeni.

3. Configure default gateway as directly connected router for Hosts Host 1 and Host 2 and traffic injectors TI1, TI2 , TI3 and TI4 for the 192.168.0.0/16 network using the following command
sudo route add default gateway -net 192.168.0.0 netmask 255.255.0.0 gw <IP of directly connected router>

4. Start the dijkstras and congestion reporter module on all the routers using the following commands
./congestion_reporter.o
./dijkstra.o

5. QOSPF routing setup is complete.




FTP Instructions:

1. Use the ftp.sh script to setup the ftp server.

2. Once, the ftp server setup is complete, from the client 

ftp <IP address of ftp server>

command can be used to connect to the server.

3. Provide the following username and password
username: john
password:john

4. USe the put <file name> command to send a file to server.





