#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/errno.h>
#include "SBCPlib.h"




int main(int argc, char const *argv[])
{
	if (argc != 4) {
        errno = EPERM;
        printf("INPUT_ERROR: ERRNO: \t%s\n", strerror(errno));
        return -1;
    }
	else
	{
		int sockfd;
		struct sockaddr_in servaddr;
		struct hostent* hret;
		fd_set master;
		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_ZERO(&master);
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd <0)
		{
			printf("socket creating failed!\n");
			exit(0);
		}
		else
		{
			printf("socket successfully created!\n");
		}
		memset(&servaddr, 0, sizeof(servaddr));
		
		servaddr.sin_family = AF_INET;
		hret = gethostbyname(argv[2]);
		memcpy(&servaddr.sin_addr.s_addr, hret->h_addr, hret->h_length);
		servaddr.sin_port = htons(atoi(argv[3]));
		if (connect(sockfd,(struct sockaddr *)&servaddr, sizeof(servaddr))<0)
		{
			printf("Server connection failed.\n");
			exit(0);
		}
		else
		{
			printf("connected to the server!\n");
			/* sendJoin(sockfd, argv); */
			//Join
			struct SBCP_header header;
			struct SBCP_attribute attr;
			struct SBCP_message msg;
			
			header.vrsn = '3';
			header.type = '2';// JOIN type: 2
			attr.type = USERNAME; // attribute: username
			attr.length = strlen(argv[1]) + 1;
			strcpy(attr.payload,argv[1]);
			msg.header = header;
			msg.attribute[0] = attr;
			write(sockfd,(void *) &msg, sizeof(msg));
			// wait for the server_reply
			sleep(1);
			struct SBCP_message serverMsg;
	
			read(sockfd, (struct SBCP_message *) &serverMsg, sizeof(serverMsg));
			if (serverMsg.header.type == FWD)
			{
				
					printf("FWD msg from %s: %s", serverMsg.attribute[1].payload, serverMsg.attribute[0].payload);
				
				
			}
			if (serverMsg.header.type == NAK)
			{
				
					printf("NAK msg from server: %s", serverMsg.attribute[0].payload);
					close(sockfd);
				
			}
			if (serverMsg.header.type == OFFLINE)
			{
				
					printf("OFFLINE msg: %s is now offline.", serverMsg.attribute[0].payload);
					
				
			}
			if (serverMsg.header.type == ACK)
			{
				
					printf("ACK msg from server: %s", serverMsg.attribute[0].payload);
				
				
			}
			if (serverMsg.header.type == ONLINE)
			{
				
					printf("ONLINE msg: %s is now online.", serverMsg.attribute[0].payload);
				
				
			}
			
			
			FD_SET(sockfd, &master);
			FD_SET(STDIN_FILENO, &master);
			while(1)
			{
				read_fds = master;
				printf("\n");
				if (select(sockfd+1, &read_fds, NULL, NULL, NULL) <0)
				{
					perror("select");
					exit(4);
				}
				if (FD_ISSET(sockfd, &read_fds))
				{
					/* getServerMsg(sockfd); */
					struct SBCP_message serverMsg;
	
					read(sockfd, (struct SBCP_message *) &serverMsg, sizeof(serverMsg));
					if (serverMsg.header.type == FWD)
					{
						
							printf("FWD msg from %s: %s", serverMsg.attribute[1].payload, serverMsg.attribute[0].payload);
						
						
					}
					if (serverMsg.header.type == NAK)
					{
						
							printf("NAK msg from server: %s", serverMsg.attribute[0].payload);
							close(sockfd);
						
					}
					if (serverMsg.header.type == OFFLINE)
					{
						
							printf("OFFLINE msg: %s is now offline.", serverMsg.attribute[0].payload);
							
						
					}
					if (serverMsg.header.type == ACK)
					{
						
							printf("ACK msg from server: %s", serverMsg.attribute[0].payload);
						
						
					}
					if (serverMsg.header.type == ONLINE)
					{
						
							printf("ONLINE msg: %s is now online.", serverMsg.attribute[0].payload);
						
						
					}
				}
				if (FD_ISSET(STDIN_FILENO, &read_fds))
				{
					struct SBCP_message msg;
					struct SBCP_attribute clientAttr;
					int readServ = 0;
					char tp[512];
					struct timeval tv;
					fd_set readfds;
					tv.tv_sec = 2;
					tv.tv_usec = 0;
					FD_ZERO(&readfds);
					FD_SET(STDIN_FILENO, &readfds);
					select(STDIN_FILENO+1, &readfds, NULL, NULL, &tv);
					if (FD_ISSET(STDIN_FILENO, &readfds))
					{
						readServ = read(STDIN_FILENO, tp, sizeof(tp));
					if (readServ > 0)
					{
						tp[readServ] = '\0';
					}
					clientAttr.type = MESSAGE;
					strcpy(clientAttr.payload, tp);
					msg.attribute[0] = clientAttr;
					write(sockfd, (void *) &msg, sizeof(msg));
					}
					else
					{
						printf("Timed out!\n");
					}
				}
			}
		
		}
	}
	
	return 0;
}

