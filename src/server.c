#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/un.h>
#include<netinet/in.h>
#include<errno.h>
#include<arpa/inet.h>

#define MAXLINE 1024
#define MAX_CLIENTS 1024

int edit(char *lan)
{	
	char save[100][512];
	char linebuffer[512] = {0};
	memset(save, 0, sizeof(save));
	int i=0;
	int k=0;
	int j=0;

	FILE *fp = fopen("/etc/config/network", "r+");
	/*FILE *fp = fopen("/home/skylab/Mywork/openwrt/network", "r+");*/
	if(fp == NULL)
	{
		printf("file open error\n");
		return -1;
	}
	
	/*copy the content of network file into cached arrays*/
	while(fgets(save[i], 512, fp))
	{
		i++;
	}

	fseek(fp,0,SEEK_SET);  /*SEEK_SET means offset from head, fp got back to head*/
	while(fgets(linebuffer,512,fp))
	{
		k++;
		if(!strcmp("config interface 'wan'\n",linebuffer))
		{
			k--;
			break;
		}
	}
	/*printf("linebuffer=%s\n",linebuffer);
	printf("k=%d\n",k);*/

	/*write buffer*/
	memset(save[k+2], 0, 512);	
	snprintf(save[k+2], 100, "\toption device '%s'\n", lan);

	/*delete the original content*/
	fseek(fp, 0, SEEK_SET);
	truncate("/etc/config/network",0);
	/*truncate("/home/skylab/Mywork/openwrt/network",0);*/

	/*write file according to the line of buffer*/
	fseek(fp, 0, SEEK_SET);
	for(j=0;j<i;j++)
	{
		fwrite(save[j], strlen(save[j]), 1, fp);
	}
	fclose(fp);
	return 0;
}


int main (int argc,char **argv)
{
	int listenfd,connfd,sockfd;
	struct sockaddr_in sockaddr;
	char buff[MAXLINE];
	int n;
	fd_set read_fds,all_fds;
	int maxfd,i,client[MAX_CLIENTS];
	
	/*Initialize client array*/
	for(i=0;i<MAX_CLIENTS;i++){
		client[i]=-1;
	}
	
	memset(&sockaddr,0,sizeof(sockaddr));

	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sockaddr.sin_port = htons(10004);

	listenfd = socket(AF_INET,SOCK_STREAM,0);

	bind(listenfd,(struct sockaddr *) &sockaddr,sizeof(sockaddr));
	
	listen(listenfd,1024);

	printf("Pls wait for the client info\n");
	
	FD_ZERO(&all_fds);
	FD_SET(listenfd, &all_fds);
	maxfd = listenfd;

	for(;;)
	{	
		read_fds = all_fds; /*copy*/
		int activity = select(maxfd+1,&read_fds,NULL,NULL,NULL);
		
		if(activity < 0 && errno!=EINTR) 
		{
            		printf("select error: %s errno: %d\n", strerror(errno), errno);
            		continue;
        	}
		
		if (FD_ISSET(listenfd, &read_fds)) 
		{ // New client connection
            		if ((connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1) /*connfd is a connected descriptor,accept() will block until connection*/
			{
                		printf("accept socket error: %s errno: %d\n", strerror(errno), errno);
                		continue;
            		}

            		printf("New client connected\n");

            		for (i = 0; i < MAX_CLIENTS; i++) 
			{
                		if (client[i] == -1) 
				{
                    			client[i] = connfd;
                    			break;
                		}
            		}

            		FD_SET(connfd, &all_fds);
            		if (connfd > maxfd) 
			{
                		maxfd = connfd;
            		}
        	}

		for (i = 0; i < MAX_CLIENTS; i++) 
		{
			sockfd = client[i];
		    	if (sockfd < 0) {
		        	continue;
			}

		    	if (FD_ISSET(sockfd, &read_fds)) 
			{ // Data from client
		        	if ((n = recv(sockfd, buff, MAXLINE, 0)) > 0) /*recv() will return the len of info*/
				{
		            		buff[n] = '\0';
		            		/*printf("recv msg from client: %s\n", buff);*/
					send(sockfd, "recv msg from client: ", strlen("recv msg from client: "), 0);
					send(sockfd, buff, strlen(buff), 0);
					send(sockfd, "\n", 1, 0);
					
					if((strcmp(buff,"lan1") && strcmp(buff,"lan2") && strcmp(buff,"lan3") && strcmp(buff,"lan4"))==1){
						/*printf("invalid msg! Command needs to be chose among lan1/lan2/lan3/lan4, pls send again\n");	*/
						send(sockfd, "invalid msg! Command needs to be chose among lan1/lan2/lan3/lan4, pls send again\n", strlen("invalid msg! Command needs to be chose among lan1/lan2/lan3/lan4, pls send again\n"), 0);				
						continue;
					}

		            		if(edit(buff)!=-1)   /*perform edit()*/
					{
		            			/*printf("'/etc/config/network' has been changed, %s is wan\n", buff);*/
						send(sockfd, "'/etc/config/network' has been changed, ", strlen("'/etc/config/network' has been changed, "), 0);
						send(sockfd, buff, strlen(buff), 0);
						send(sockfd, " is wan\n", strlen(" is wan\n"), 0);

						/*network restart according to the command from client*/
						/*printf("Do u want to restart the network to validate your edit?(u can send msg again when restart is done)[y/n]:\n");*/
						send(sockfd, "Do u want to restart the network to validate your edit?(u can send msg again when restart is done)[y/n]:\n", strlen("Do u want to restart the network to validate your edit?(u can send msg again when restart is done)[y/n]:\n"), 0);
						int restart_msg_len;
						if ((restart_msg_len = recv(sockfd, buff, MAXLINE, 0)) > 0) {
                            				buff[restart_msg_len] = '\0';
                            				if (buff[0] == 'y') {
                                				if (system("/etc/init.d/network restart") != -1) {
                                   					/*printf("network restarted\n");  /*I dont know how to indicate the restarting process is done, because there are many stuff in SecureCRT after run /etc/init.d/network restart*/
									send(sockfd, "network restarted\n", strlen("network restarted\n"), 0);
                                				} else {
                                    					/*printf("restart failed\n");*/
									send(sockfd, "restart failed\n", strlen("restart failed\n"), 0);
                                				}
                            				} else {
                                				/*printf("only changed the network file without restarting\n");
								printf("Now u can send lan1/lan2/lan3/lan4 again\n");*/
								send(sockfd, "only changed the network file without restarting\nNow u can send lan1/lan2/lan3/lan4 again\n", strlen("only changed the network file without restarting\nNow u can send lan1/lan2/lan3/lan4 again\n"), 0);
                            		      		}
                        			} else {
                            				printf("recv error: %s errno: %d\n", strerror(errno), errno);
                        			}
                    			}

		        	} else if (n == 0) {
		            		printf("Client disconnected\n");
		            		close(sockfd);
		            		FD_CLR(sockfd, &all_fds);
		            		client[i] = -1;
		        	} else {
		           	 	printf("recv error: %s errno: %d\n", strerror(errno), errno);
		            		close(sockfd);
		            		FD_CLR(sockfd, &all_fds);
		            		client[i] = -1;
                		}
            		}
        	}
	}

    close(listenfd);

    return 0;
}
