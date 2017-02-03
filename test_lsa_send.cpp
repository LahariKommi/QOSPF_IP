#include <stdlib.h>
#include <string.h>
#include <string.h>
#include<stdio.h>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>

#define serverPort 7899



int main(int argc, char *argv[])
{
	struct in_addr a;
	int clientSocket;
	int messageToSend[4];
	int message[4];

	struct sockaddr_in serverAddr;
	int LinkCost = atoi(argv[2]);
	//converting interface IP from str to integer

	inet_pton(AF_INET, argv[1], &a);

	/*Create UDP socket*/
	clientSocket = socket(PF_INET, SOCK_DGRAM, 0);

	//trying to connect to two servers using same socket.{
	/*Configure settings in address struct*/
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPort);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

	message[0]=a.s_addr;
	message[1]=255;
	message[2]=255;
	message[3]=LinkCost;
	int i=0;
	for (i = 0 ; i < 4 ; ++i) {
		messageToSend[i] = htonl(message[i]) ;
	}
	sendto(clientSocket, messageToSend, sizeof(messageToSend), 0, (struct sockaddr *)&serverAddr,sizeof(serverAddr));

	printf("New LSA Sent with following Details\n");
	printf("Node: %s\n", argv[1]);
	printf("Link Cost: %d\n",message[3]);

}

