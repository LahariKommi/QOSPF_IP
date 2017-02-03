//Congestion Reporter Code

#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include<stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <string.h>


#define MTA_SIZE 10
#define costCalInterval 1
#define interfaceBandwidth 10000000 //10Mbps
#define OSPFRefBw 100000000//100MBps
#define serverPort 7899
#define CongestionWeight 500

//Defining congestion windows.

#define Window1u 40.0
#define Window2u 65.0
#define Window3u 90.0
#define Window4u 110.0
#define Window1l 0.0
#define Window2l 30.0
#define Window3l 55.0
#define Window4l 80.0



#define NumberofWindows 4

//interface structure
typedef struct Interface {

   char nameInt[15];
   char IPint[15];
int integerIP;
   long prevtxBytes[MTA_SIZE];

 unsigned long prevBytes;
   long  MTA_avg;

float Cost;
float prevLSACost;
int prevLSAwin;
 
}Interface;

struct Window{

float UpThres;
float LowerThres;


}Window;

void InitialiseWindows(struct Window *ptrWindow)
{
int i;
float WindowThresholds[NumberofWindows*2]={Window1u,Window1l,Window2u,Window2l,Window3u,Window3l,Window4u,Window4l};
for (i=0; i<NumberofWindows;i++)
{
 (ptrWindow+i)->UpThres=WindowThresholds[2*i];
(ptrWindow+i)->LowerThres=WindowThresholds[(2*i)+1];
}
}

int NumberofInterfaces()
{	
	//printf("You are in  NumberofInterfaces\n");
	  FILE *netdevfp;
          netdevfp=fopen("/proc/net/dev","r");
          size_t len=0;
          char *eachline, *token;
	  int numInterfaces=0;
	  
          while (getline(&eachline, &len, netdevfp)!=-1) {
		
              if (strchr(eachline,'|')==NULL) 
	      {
		numInterfaces=numInterfaces+1;
		
	     }
	}
	//printf("Number of Interfaces: %d\n", numInterfaces);
	return numInterfaces;
}


void sendLSA(char *IP,int cost)
{
struct in_addr a; 
//printf("YOU ARE IN SEND LSA\n");
  int clientSocket, portNum, nBytes,LSABytes;
  int messageToSend[4];
  int message[4];

 
  struct sockaddr_in serverAddr;
  socklen_t addr_size;

  int LinkCost;

int intIP;
//converting interface IP from str to integer

inet_pton(AF_INET, IP, &a);
//printf("%d\n", a.s_addr);

  /*Create UDP socket*/
  clientSocket = socket(PF_INET, SOCK_DGRAM, 0);

//trying to connect to two servers using same socket.{
  /*Configure settings in address struct*/
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(serverPort);
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

  /*Initialize size variable to be used later on*/
  addr_size = sizeof serverAddr;

//Creating the LSA message


LinkCost=cost;

message[0]=a.s_addr;
message[1]=255;
message[2]=255;
message[3]=LinkCost;
int i=0;
for (i = 0 ; i < 4 ; ++i) {
    messageToSend[i] = htonl(message[i]) ;
}
sendto(clientSocket, messageToSend, sizeof(messageToSend), 0, (struct sockaddr *)&serverAddr,addr_size);
  
printf("New LSA Sent with following Details\n");
printf("Node: %d\n",message[0]);
printf("Sequence Number:  %d\n",message[1]);
printf("Link ID: %d\n",message[2]);
printf("Link Cost: %d\n",message[3]);

}


void CalculateCost(struct Interface *ptrIntr,int intNum, struct Window *ptrWindow)
{
float availableBw,availableBw2;
float cost;
int costInt;
int i,win,k;
for (i=0;i<intNum;i++)//for each interface
	{	
		//available bandwidth to calculate
		if((strcmp((ptrIntr+i)->nameInt,"lo") !=0  ) &&(strcmp((ptrIntr+i)->nameInt,"eth0") !=0  ))
	{
		
		availableBw2=interfaceBandwidth-(((ptrIntr+i)->MTA_avg*8)/(MTA_SIZE*costCalInterval));
		//printf("Available Bandwidth: %f\n",availableBw2);
		//cost=(OSPFRefBw/interfaceBandwidth)+(CongestionWeight*(OSPFRefBw/availableBw2));//scaled by a factor of 4.
		cost=(OSPFRefBw/interfaceBandwidth)+(CongestionWeight*((interfaceBandwidth-availableBw2)/interfaceBandwidth));//scaled by a factor of 4.
		//printf("OSPFRefBw/availableBw2: %f\n",OSPFRefBw/availableBw2);
		//printf("CALCULATED COST: %f\n",cost);
		

		//Using different Congestion Windows

		//win=(cost-10)/25;
		win=(ptrIntr+i)->prevLSAwin;
		printf("Interface : %s\n", (ptrIntr+i)->nameInt);
		printf("Current Congestion Window: %d\n",win);
		printf("Prev Congestiion Window: %d\n",(ptrIntr+i)->prevLSAwin);
		if(cost<(ptrWindow+win)->LowerThres) 
		{
		
		
			printf	("Congestion window decreased. Sending LSA...\n")	;	
			sendLSA((ptrIntr+i)->IPint,cost);
			(ptrIntr+i)->prevLSAwin=(ptrIntr+i)->prevLSAwin-1;
		}
		
		else if(cost>(ptrWindow+win)->UpThres)
		{
			printf	("Congestion window increased. Sending LSA...\n")	;	
			sendLSA((ptrIntr+i)->IPint,cost);
			(ptrIntr+i)->prevLSAwin=(ptrIntr+i)->prevLSAwin+1;
		}
		
		

		(ptrIntr+i)->Cost=cost;
	}
	}

}
void arrayArranger(struct Interface *ptrIntr,int intNum)
{
	//printf("You are in  arrayArranger\n");	
int k,i=0;
	for (i=0;i<intNum;i++)
	{	
if((strcmp((ptrIntr+i)->nameInt,"lo") !=0  ) &&(strcmp((ptrIntr+i)->nameInt,"eth0") !=0  ))
{
		for (k =0; k<MTA_SIZE-1;k++)
		{
			(ptrIntr+i)->prevtxBytes[k]=(ptrIntr+i)->prevtxBytes[k+1];
			
		}
		(ptrIntr+i)->prevtxBytes[MTA_SIZE-1]=0;
}
		
	}
}



void MTAcomputer(struct Interface *ptrIntr,int intNum)
{
	int i,k;
	//printf("You are in  MTAcomputer\n");
	long Sum=0;	
	for (i=0;i<intNum;i++)
	{       

if((strcmp((ptrIntr+i)->nameInt,"lo") !=0  ) &&(strcmp((ptrIntr+i)->nameInt,"eth0") !=0  ))
{
		//printf("Interface: %d\n",i+1);	
            	Sum=0;
		
		for (k =0; k<MTA_SIZE;k++)
		{
			
			Sum=Sum+(ptrIntr+i)->prevtxBytes[k];
			
			//printf("Bytes %ld\n",(ptrIntr+i)->prevtxBytes[k]);
			
			//printf("Sum :%lu\n", Sum);
			
		}
		(ptrIntr+i)->MTA_avg=(Sum/MTA_SIZE);
		
		//printf("MTA_avg :%lu\n", (ptrIntr+i)->MTA_avg);
}
	}
}
void interfaceInitialiser(struct Interface *ptrIntr)
{
//printf("You are in  interfaceInitialiser\n");
          FILE *netdevfp;
          netdevfp=fopen("/proc/net/dev","r");
          size_t len=0;
          char *eachline, *token, *ptr, *interfaceName, *intCurr, command[300], *IPcurr, name[15],IP[15];
	  int i,intNumPtrVar,intNumCurr=0;
	  unsigned long valueread, txBytesCurr;
          int index=0;
	   int family, s;
    char host[NI_MAXHOST];
		struct ifaddrs *ifaddr, *ifa;
          while (getline(&eachline, &len, netdevfp)!=-1) {
		
              if (strchr(eachline,'|')==NULL) 
	      {
		// printf("%s",eachline);
                  token=strtok(eachline,":");
		  i=1;
		  index=0;
		intNumCurr=intNumCurr+1;//calculate the interface number.
		intNumPtrVar=intNumCurr-1;//pointer will point to first interface. There onwards you need to point to base+1 for 2nd interface and base+2 for third and so on.
		  while(token !=NULL)
		{ 	
			while(token[0]==' ')//to eliminate all null spaces before interface name.
			{
				token++;
			}

			if(i==1)// as each line starts with interface name
			{
				strcpy(name,token);
				//printf("InterfaceName: %s\n",name);
				intCurr=name;
				strcpy((ptrIntr+intNumPtrVar)->nameInt, intCurr);
				
				
				//to get interface IP

    if (getifaddrs(&ifaddr) == -1) 
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }


    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
    {
        if (ifa->ifa_addr == NULL)
            continue;  

        s=getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        if((strcmp(ifa->ifa_name,intCurr)==0)&&(ifa->ifa_addr->sa_family==AF_INET))
        {
            if (s != 0)
            {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
           // printf("\tInterface : <%s>\n",ifa->ifa_name );
            //printf("\t  Address : <%s>\n", host); 
			strcpy(IP,host);
	    strcpy((ptrIntr+intNumPtrVar)->IPint, IP);
        }
    }

				i=i+1;
			}
			token = strtok (NULL, ":");
		}
			 freeifaddrs(ifaddr);
	     }
	}

}
void measureCongestion(struct Interface *ptrIntr)
{
 

	// printf("You are in  measureCongestion\n");
          FILE *netdevfp;
          netdevfp=fopen("/proc/net/dev","r");
          size_t len=0;
          char *eachline, *token, *ptr, *interfaceName, *intCurr, command[300], *IPcurr, name[15],IP[15];
	  int i,intNumPtrVar,intNumCurr=0;
	  unsigned long valueread, txBytesCurr;
          int index=0;
	  int skip=0;
          while (getline(&eachline, &len, netdevfp)!=-1) {
		
              if (strchr(eachline,'|')==NULL) 
	      {
		// printf("%s",eachline);
                  token=strtok(eachline,":");
		  i=1;
		  index=0;
		intNumCurr=intNumCurr+1;//calculate the interface number.
		intNumPtrVar=intNumCurr-1;//pointer will point to first interface. There onwards you need to point to base+1 for 2nd interface and base+2 for third and so on.
		  while(token !=NULL)
		{ 	
			while(token[0]==' ')//to eliminate all null spaces before interface name.
			{
				token++;
			}
			

			//to measure the output bytes
			
                 	while((token=strtok(NULL," "))!=NULL) 
			{
                  		valueread=strtoul(token,&ptr,10);
				
                          	if ((index==8 )&& ((strcmp((ptrIntr+intNumPtrVar)->nameInt,"lo") !=0  ) && (strcmp((ptrIntr+intNumPtrVar)->nameInt,"eth0") !=0  )))
				{
					
					
					txBytesCurr=valueread;
					//printf("Tx Bytes: %ld\n",txBytesCurr);
					(ptrIntr+intNumPtrVar)->prevtxBytes[MTA_SIZE-1]=txBytesCurr;//Total bytes transferred till current interval
(ptrIntr+intNumPtrVar)->prevtxBytes[MTA_SIZE-1]=txBytesCurr-((ptrIntr+intNumPtrVar)->prevBytes);//subtract from the number of bytes transmitted in prev slot to calculate congestion.

					(ptrIntr+intNumPtrVar)->prevBytes=txBytesCurr;
				
				}
                          	index++;
                      	}
			token = strtok (NULL, ":");
		}
   
              }
          }
          fclose(netdevfp);
}         
    
void main()
{
//printf("You are in  main\n");
int intNum=0;
struct Interface *ptrIntr;
struct Window *ptrWindow;
intNum=NumberofInterfaces();
bool bootup=1;
int bootupVar=0;

int i;
ptrIntr = (struct Interface*) malloc(intNum * sizeof(struct Interface));
for(i=0;i<intNum;i++)
{
memset(&(ptrIntr+i)->prevtxBytes,0,sizeof(&(ptrIntr+i)->prevtxBytes));

//all arrays are being set to 0;
(ptrIntr+i)->prevBytes=0;

}
interfaceInitialiser(ptrIntr);

ptrWindow = (struct Window*) malloc(NumberofWindows * sizeof(struct Window));
InitialiseWindows(ptrWindow);


while(1)
{
while(bootup==1)
{//printf("Booting UP...");
// bootup time introduced to let all the MTA_SIZE positions in array to get filled.Till this time no congestion will be calculated.
arrayArranger(ptrIntr,intNum);

measureCongestion(ptrIntr);

bootupVar++;
if(bootupVar==MTA_SIZE)
{//all locations filled. 
bootup=0;
}

}

//bootup time is over so normal process.
arrayArranger(ptrIntr,intNum);

measureCongestion(ptrIntr);

MTAcomputer(ptrIntr,intNum);


CalculateCost(ptrIntr,intNum,ptrWindow);

printf("Displaying Information:\n");
printf("Interface Name\tInterface IP\tMovingTimeAvgBytes\tCost\n");
for(i = 0; i < intNum; ++i)
       printf("%s\t\t%s\t\t%lu\t\t%f\n", (ptrIntr+i)->nameInt, (ptrIntr+i)->IPint,(ptrIntr+i)->MTA_avg,(ptrIntr+i)->Cost);
printf("\n\n");
sleep(costCalInterval);


}
 
}
