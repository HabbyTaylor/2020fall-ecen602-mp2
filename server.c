#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "SBCPlib.h"





int totalClientNum = 0;//compute the total client number
struct SBCP_client_info *clientLst;//the the list of all clients with their information

//checks if username exist already
int checkUsername(char a[], int maxClientNum){//
	int i = 0;
	while(i<totalClientNum){//go through all recorded clients fd
		if(!strcmp(a,clientLst[i].username)){//compare the new username whether is existed in our username list
			return 2;//yes! we have username existed
		}
		if(totalClientNum == maxClientNum){
			return 1;
		}
		i++;
	}
	return 0;//no! we don`t have username existed
}





void broadcast(struct SBCP_message broadcastMsg,fd_set RecordedClientFdLst,int listenFd,int acceptFd, int latestFd){
	int i=0;
	while(i<=latestFd){
		if (FD_ISSET(i, &RecordedClientFdLst)) //check the broadcasting fd is valid or not
		{
		// except the listener and ourselves
			if(listenFd!=i&&acceptFd!=i)
			{
				if ((write(i,(void *) &broadcastMsg,sizeof(broadcastMsg))) == -1)
				{
					printf("Fail. Server cannot broadcast.\n");
				}
			}
		}
		i++;
	}
	
}

int main(int argc, char const *argv[])
{
	struct SBCP_message sendMsg;
	struct SBCP_attribute sendMsg_attribute;
	struct SBCP_message broadcastMsg;
	int acceptFd;
	int usernameFlag = 0;
	struct sockaddr_in clientInfo;
	fd_set RecordedClientFdLst;//this is the set of descriptors, which is in the server`s local list
	fd_set monitorFdLst;//this is the monitoring set of descriptors
	int latestFd;//recordeing the latest client socket
	int temp;
	int x,y;
	int currFd;
	int index_r;
	int maxClientNum=0;
	
	
	struct sockaddr_in serverAddress;
	struct hostent* hret;
	int listenFd;
	listenFd = socket(AF_INET,SOCK_STREAM,0);//create the server socket
	printf("Server socket created. \n");
	memset(&serverAddress,0,sizeof(serverAddress));
	
	serverAddress.sin_family = AF_INET;
	hret = gethostbyname(argv[1]);
	memcpy(&serverAddress.sin_addr.s_addr, hret->h_addr,hret->h_length);
	serverAddress.sin_port = htons(atoi(argv[2]));
	bind(listenFd, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
	printf("Server socket binded. \n");
	listen(listenFd, 99);
	printf("Server is listening... \n");
	
	maxClientNum=atoi(argv[3]);//the maximum number of client socket
	clientLst= (struct SBCP_client_info *)malloc(maxClientNum*sizeof(struct SBCP_client_info));//set engough space for the client list
	FD_SET(listenFd, &RecordedClientFdLst);//put the server socket into our local recorded list
	latestFd = listenFd;//the initial value of our latest socket is the server socket
	
	for(;;)
	{
		monitorFdLst = RecordedClientFdLst;//set the monitoring descriptors list as what we recorded in local
		select(latestFd+1, &monitorFdLst, NULL, NULL, NULL);
		currFd=0;
		while(currFd<=latestFd)
		/* for(currFd=0 ; currFd<=latestFd ; currFd++) *///the server will scan all fd to see which fd has made changes in the monitoring fd list.
		{
			if(FD_ISSET(currFd, &monitorFdLst))//to verify the fd we monitoring is the valid fd, which is in our monitoing list.
			{
				if(currFd == listenFd)//if the changed fd is our server socket, that means there`s a new socket coming to connect, which is the new client want to JOIN
				{
					int clientSize = sizeof(clientInfo);
					acceptFd = accept(listenFd,(struct sockaddr*)&clientInfo,&clientSize);
					if(acceptFd < 0){printf("Server acception failed.\n");}
					else if(acceptFd>=0)
					{
						temp = latestFd;//to record the latestFd
						FD_SET(acceptFd, &RecordedClientFdLst);//put the new client socket fd into our local recorded client fd list
						if(acceptFd > latestFd){//to make sure this is the largest fd we have now
						latestFd = acceptFd;//
						}
						struct SBCP_message joinMsg;
						struct SBCP_attribute joinMsgAttribute;
						char indexMsg[16];
						read(acceptFd,(struct SBCP_message *) &joinMsg,sizeof(joinMsg));//read what`s the message from the client
						joinMsgAttribute = joinMsg.attribute[0];//get the first attribute in the message, which will include the username
						strcpy(indexMsg, joinMsgAttribute.payload);//remember to only operate the message in the indexMsg, so the message in the joinMsg would not change
						
						
						usernameFlag = checkUsername(indexMsg, maxClientNum);
						if(usernameFlag == 0)//0 means there`s no username existed, so this username is availeble
						{
							strcpy(clientLst[totalClientNum].username, indexMsg);//update the new client into the client list
							clientLst[totalClientNum].fd = acceptFd;
							
							totalClientNum ++;//our total count of the clients + 1
							//send ACK message to this client
							/* sendAcknowledgement(acceptFd); */
							struct SBCP_message ACKMsg;
							struct SBCP_header ACKMsgHeader;
							struct SBCP_attribute ACKMsgAttr;
							int i = 0;
							char a[128];
							a[0] = (char)(((int)'0')+ totalClientNum);//the order of new client in the server
							a[1] = ' ';
							a[2] = '\0';//
							while(i<totalClientNum-1){
								strcat(a,clientLst[i].username);//copy and paste all client`s username in our recorded client list into the message, as the ACK message attribute payload
								strcat(a, ",");
								i++;
							}
							
							ACKMsgHeader.vrsn=FWD;//protocol version
							ACKMsgHeader.type=ACK;//7 is the ACK message
							ACKMsgAttr.type = MESSAGE;//the payload in the attribute is message
							
							ACKMsgAttr.length = strlen(a)+1;//the length of the attribute is the size of message plus 1
							strcpy(ACKMsgAttr.payload, a);
							ACKMsg.header = ACKMsgHeader;
							ACKMsg.attribute[0] = ACKMsgAttr;
							write(acceptFd,(void *) &ACKMsg,sizeof(ACKMsg));
							
							//broadcast ONLINE message to all clients except this client
							printf("New client (%s) is in the chat room.\n",clientLst[totalClientNum-1].username);
							broadcastMsg.header.vrsn=3;//the protocol version
							broadcastMsg.header.type=ONLINE;//8 means ONLINE message
							broadcastMsg.attribute[0].type=USERNAME;//type 2 attribute means username in the payload
							strcpy(broadcastMsg.attribute[0].payload,clientLst[totalClientNum-1].username);
							broadcast(broadcastMsg,RecordedClientFdLst,listenFd,acceptFd, latestFd);
						}
						//the new connection has the username existed, the username is not available
						else
						{
							if(usernameFlag == 1){
								printf("Max number of users exceeded. Connection denied. \n");
								
								struct SBCP_message NAKMsg;
								struct SBCP_header NAKMsg_header;
								struct SBCP_attribute NAKMsg_attribute;
								char temp[128];
								NAKMsg_header.vrsn =FWD;//protocol version is 3
								NAKMsg_header.type =NAK;//indicate the sbcp message type, 5 is the NAK message
								NAKMsg_attribute.type = REASON;//1 means the attribute`s payload include the reason of failure
								
								strcpy(temp,"Server has reached the maximum client number.\n");
								
								
								NAKMsg_attribute.length = strlen(temp);//the length of the payload is depends on the message we write
								strcpy(NAKMsg_attribute.payload, temp);
								NAKMsg.header = NAKMsg_header;
								NAKMsg.attribute[0] = NAKMsg_attribute;
								write(acceptFd,(void *) &NAKMsg,sizeof(NAKMsg));//send the NAK message to the new client
								close(acceptFd);
								
							}
							else{
								printf("This username has already existed. Connection denied. \n");
								
								struct SBCP_message NAKMsg;
								struct SBCP_header NAKMsg_header;
								struct SBCP_attribute NAKMsg_attribute;
								char temp[128];
								NAKMsg_header.vrsn =FWD;//protocol version is 3
								NAKMsg_header.type =NAK;//indicate the sbcp message type, 5 is the NAK message
								NAKMsg_attribute.type = REASON;//1 means the attribute`s payload include the reason of failure
								
								strcpy(temp,"Username has already existed.\n")
										
								
								
								NAKMsg_attribute.length = strlen(temp);//the length of the payload is depends on the message we write
								strcpy(NAKMsg_attribute.payload, temp);
								NAKMsg.header = NAKMsg_header;
								NAKMsg.attribute[0] = NAKMsg_attribute;
								write(acceptFd,(void *) &NAKMsg,sizeof(NAKMsg));//send the NAK message to the new client
								close(acceptFd);
							}
							latestFd = temp;//return the latest fd to the previous one
							FD_CLR(acceptFd, &RecordedClientFdLst);//clear acceptFd out of the recorded client list
						}
					}
				}
				//this is not a new connection, means the existed fd has change
				else
				{
					if ((index_r=read(currFd,(struct SBCP_message *) &sendMsg,sizeof(sendMsg))) <= 0)
					{//the error or 0 situation will cause the server close the socket
						int tp=0;
						if (index_r == 0) //client close socket, the server will receive 0 from the client
						{
							//broadcast the OFFLINE message to all other clients
							y=0;
							while(y<totalClientNum+1){
								if(clientLst[y].fd==currFd)
								{
									tp=y;
									broadcastMsg.attribute[0].type=2;//means the message attribute payload is username
									strcpy(broadcastMsg.attribute[0].payload,clientLst[y].username);
									printf("Client (%s) has left the chat room.\n", clientLst[y].username);
								}
								y++;
							}
							
							broadcastMsg.header.vrsn=3;
							broadcastMsg.header.type=OFFLINE;//offline message type
							broadcast(broadcastMsg,RecordedClientFdLst,listenFd,currFd, latestFd);
						}//end of the offline broadcast
						else if(index_r < 0)
						{
							printf("Server reading failed. \n");
						}
						close(currFd); // close the client socket
						FD_CLR(currFd, &RecordedClientFdLst); // remove from RecordedClientFdLst set
						
						while(tp<totalClientNum){
							clientLst[tp]=clientLst[tp+1];
							tp++;
						}
						totalClientNum--;//the total number of clients -1
					}//end of error or close sitaution
					//the message is coming from one client
					else
					{
						sendMsg_attribute = sendMsg.attribute[0];
						broadcastMsg=sendMsg;//copy the whole message which is going to FWD
						broadcastMsg.header.type=FWD;//3 means FWD, which original is 4 as the SEND message
						broadcastMsg.attribute[1].type=USERNAME;
						int payloadLength=0;
						char temp[128];
						payloadLength=strlen(sendMsg_attribute.payload);
						strcpy(temp,sendMsg_attribute.payload);
						temp[payloadLength]='\0';
						int i=0;
						while(i<totalClientNum){
							if(clientLst[i].fd==currFd)
							{
								strcpy(broadcastMsg.attribute[1].payload,clientLst[y].username);
								printf("FWD message from client (%s) is: %s \n",clientLst[y].username,temp);
							}
							i++;
						}
						broadcast(broadcastMsg,RecordedClientFdLst,listenFd,currFd, latestFd);
					}
				}
			}
			currFd++;
		}
	}
	close(listenFd);
	return 0;
}

