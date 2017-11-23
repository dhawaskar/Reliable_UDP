#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stddef.h>
#include <memory.h>
#include <string.h>
/* You will have to modify the program below */

#define MAXBUFSIZE 100
#define KEY 0XAA

//encypting the message with key
void encrypt_data(char *message,int len){
	int i;
	printf("Encrypting the message\n");
        for(i = 0; i <len; i++)
                message[i] ^= KEY;

}

//decrypting the message with key
void decrypt(char *message,int len){
	int i;
	printf("Decrypting the message\n");
        for(i = 0; i <len; i++)
                message[i] ^= KEY;

}

int  server_send(int sock,char buffer[],int buffer_len,struct sockaddr_in remote,int remote_length){
	int nbytes;
	if((nbytes=sendto(sock,buffer,buffer_len,0,(struct sockaddr *)&remote,remote_length))<0){
		perror("send to client function failed\n");
		return -1;
	}
	else{
		return nbytes;
	}
}

int main (int argc, char * argv[] )
{
	int sock;                           //This will be our socket
	FILE *fp;
	struct sockaddr_in sin, remote;     //"Internet socket address structure"
	unsigned int remote_length;         //length of the sockaddr_in structure
	int nbytes;                        //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	char message[MAXBUFSIZE];
	char message1[MAXBUFSIZE];
	char invalid_command[MAXBUFSIZE];
	char com1[]="ls",com2[]="put",com3[]="get",com4[]="delete",com5[]="exit",*command,*filename;
	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	/******************
	  This code populates the sockaddr_in struct with
	  the information about our socket
	 ******************/
	bzero(&sin,sizeof(sin));                    //zero the struct
	sin.sin_family = AF_INET;                   //address family
	sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine
	
	//We don't want recvfrom stuck forever hence we are making it unblock socket
       	struct timeval read_timeout;
       	read_timeout.tv_sec = 10;
       	read_timeout.tv_usec = 10;
       	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
	// done with setting timeout for socket

	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET,SOCK_DGRAM,0)) < 0)
	{
		//printf("unable to create socket");
		perror("Failed to create a socket at server side");
		exit(1);
	}
	/******************
	  Once we've created a socket, we must bind that socket to the 
	  local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		perror("Unable to bind the socket");
		printf("Run server with different port number >5000");
		exit(1);
	}

	remote_length = sizeof(remote);
	while(1){
		//waits for an incoming message
		bzero(buffer,sizeof(buffer));
		bzero(message,sizeof(message));
		if((nbytes = recvfrom(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *)&remote,&remote_length))<0){
			perror("receive from in client FAILED:");
		}
		
		strcpy(message,buffer);
		strcpy(invalid_command,buffer);
		strcpy(message1,buffer);
		command=strtok(message," \n\t");
		filename=strtok(message1," \t");
		filename=strtok(NULL,"\n");
		if(strcmp(command,com1)==0){	
				bzero(message,sizeof(message));
				printf("client is asking to list all the files\n");
				fp = popen("ls -l","r");
				while(fgets(message,sizeof(message)-1,fp)!=NULL){
					server_send(sock,message,sizeof(message),remote,remote_length);
				}
				server_send(sock,"ACK",3,remote,remote_length);
				pclose(fp);
		}
		else if(strcmp(command,com2)==0){
				char md5sum[100];
                                int nbytes_md,file_chunk_size,chunk_size=0,bytes_left,bytes_written;
				int chunks;
				FILE *fp_md;
				char end_buffer[]="ACK";
				//char filename[]="server_received";
				remove(filename);
				bzero(message,sizeof(message));
				printf("Client wants to create a local copy of the file %s on the server\n",filename);
				if((fp=fopen(filename,"wb+"))==NULL){
                                        perror("Cannot open file\n");
                                }
				//receive the file size  of the file before listening to the file
				while(1){
					bzero(message,MAXBUFSIZE);
					if((nbytes = recvfrom(sock,message,MAXBUFSIZE,0,(struct sockaddr *)&remote,&remote_length))<0)
                                        	printf("\nFailed to receive packet from server\n");
					file_chunk_size=atoi(message);
					printf("Received file chunk_size is %d\n",file_chunk_size);
					if(file_chunk_size>0){
						printf("sucessfully received file size\n");
						server_send(sock,"ACK",3,remote,remote_length);
						break;
					}
					else{
						printf("Failed to receive the file size and request again\n");
						server_send(sock,"NEGACK",6,remote,remote_length);
					}
				}
				//receive the fragments from the client
				if(file_chunk_size>MAXBUFSIZE){
					printf("file chunk_size is bigger than buffer chunk_size and hence needs fragmentation\n");
				}
				chunks=file_chunk_size/MAXBUFSIZE;
				printf("Total number of chunks:%d\n",chunks);
				fseek(fp,0,SEEK_SET);
				while(chunks--){
					bzero(message,MAXBUFSIZE);
					while(1){
						bzero(message,MAXBUFSIZE);
			        		if((nbytes = recvfrom(sock,message,MAXBUFSIZE,0,(struct sockaddr *)&remote,&remote_length))<0)
                       		 			perror("Failed to receive packet from server");
						printf("bytes received%d\n",nbytes);
						if(nbytes==MAXBUFSIZE){
							printf("received fragment and sending the ACK\n");
							server_send(sock,"ACK",3,remote,remote_length);
							break;
						}
						else {
							printf("Failed to receive the packet and hence request again\n");
							server_send(sock,"NEGACK",6,remote,remote_length);
						}
					}
					decrypt(message,nbytes);
					if((bytes_written=fwrite(message,1,nbytes,fp))<0){
						printf("Write to file from buffer failed\n");
					}
					printf("bytes written %d\n",bytes_written);
					chunk_size+=nbytes;
					fseek(fp,chunk_size,SEEK_SET);
				}
				bytes_left=file_chunk_size-chunk_size;
				printf("%d bytes left to receive\n",bytes_left);
				bzero(message,MAXBUFSIZE);
				while(1){
					bzero(message,MAXBUFSIZE);
					if((nbytes=recvfrom(sock,message,bytes_left,0,(struct sockaddr *)&remote,&remote_length))<0) perror("failed for reading the last few bytes");
					printf("bytes received %d\n",nbytes);
					if(nbytes==bytes_left){
                                                        printf("received fragment and sending the ACK\n");
                                                        server_send(sock,"ACK",3,remote,remote_length);
                                                        break;
                                                }
                                                else {
                                                        printf("Failed to receive the packet and hence request again\n");
                                                        server_send(sock,"NEGACK",6,remote,remote_length);
                                                }
				}
				decrypt(message,bytes_left);
				if((bytes_written=fwrite(message,1,bytes_left,fp))<0) perror("Writing the last few bytes failed");
				printf("bytes written %d\n",bytes_written);
				fclose(fp);
				//receive the md5sum
				bzero(message,MAXBUFSIZE);
				if((nbytes_md = recvfrom(sock,message,100,0,(struct sockaddr *)&remote,&remote_length))<0)
                                	printf("\nFailed to receive file md5sum from server\n");
				char open_com[50];
				strcpy(open_com,"md5sum ");
				strcat(open_com,filename);
                                fp_md = popen(open_com,"r");
                                while(fgets(md5sum,sizeof(md5sum)-1,fp_md)!=NULL);
                                strtok(md5sum," ");
                                printf("Computed MD5 sum is %s and expected md5sum is %s\n",md5sum,message);                              
                                fclose(fp_md);
                                if(strcmp(md5sum,message)==0) {
					printf("Yes files are identical\n");
					server_send(sock,"ACK",3,remote,remote_length);
				}
                                else {
					printf("Sorry files are not identical\n");
					server_send(sock,"NEGACK",6,remote,remote_length);
				}
		}
		else if(strcmp(command,com3)==0){
				printf("Client is requesting for the file %s\n",filename);
				int size_red=0;
				//compute the md5sum for requested file
				char command_md5[100],response[100];
				FILE *fp_md;
				int fragments=0,chunk_size=0,bytes_left,bytes_red,bytes_sent;
				strcpy(command_md5,"md5sum ");
				strcat(command_md5,filename);
				fp_md=popen(command_md5,"r");
                                while(fgets(response,sizeof(response)-1,fp_md)!=NULL);
                                strtok(response," ");
                                fclose(fp_md);
				fprintf(stdout,"MD5sum of the file is %s\n",response);


				//send the file
				if((fp=fopen(filename,"rb"))==NULL){
					perror("Cannot open file it  does not exists\n");
					printf("\nRequest for something that exists\n");
					server_send(sock,"4",1,remote,remote_length);
					bzero(buffer,MAXBUFSIZE);
					recvfrom(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *)&remote,&remote_length);
					server_send(sock,"No file",7,remote,remote_length);
					bzero(buffer,MAXBUFSIZE);
					recvfrom(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *)&remote,&remote_length);
					server_send(sock,"md5sum",1,remote,remote_length);
				}
				else{
					fseek(fp,0,SEEK_END);
  					size_t file_size=ftell(fp);
  					fseek(fp,0,SEEK_SET);
					//send the file size to client
					bzero(message,sizeof(message));
					sprintf(message,"%d",file_size);
					// sending the file size
					while(1){
						if(server_send(sock,message,sizeof(message),remote,remote_length)<0) perror("Server failed to send the file size"); 
						printf("File size sent from server\n");
						bzero(buffer,MAXBUFSIZE);
						if((nbytes=recvfrom(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *)&remote,&remote_length))<0) perror("Failed to receive ACK for file size");
						if(nbytes==3){
							printf("ACK for the file size received\n");
							break;
						}
						else{
							printf("Failed to receive the ACK and hence sending the file size again\n");
						}
					}
					//sending the file					
					printf("Sending the file\n");
					fragments=file_size/MAXBUFSIZE;
					printf("Total fragments to send %d\n",fragments);
					while(fragments--){
						if((bytes_red=fread(message,1,MAXBUFSIZE,fp))<0) perror("fread failed");
						printf("%d bytes red\n",bytes_red);
						encrypt_data(message,bytes_red);
						while(1){
							if((bytes_sent=server_send(sock,message,bytes_red,remote,remote_length))<0) perror("file chunk sent to client failed\n");
							printf("%d bytes sent\n",bytes_sent);
							bzero(buffer,MAXBUFSIZE);
							if((nbytes=recvfrom(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *)&remote,&remote_length))<0)	perror("failed to receive ACK for sent packet");
							if(nbytes==3){
								printf("Received ACK for sent fragment and will send next packet\n");
								break;
							}
							else printf("Failed to get ACK for sent packet and send again\n");
						}
						chunk_size+=bytes_red;
						fseek(fp,chunk_size,SEEK_SET);
					}
					bytes_left=file_size-chunk_size;
					printf("%d bytes are left to send\n",bytes_left);
					bzero(message,MAXBUFSIZE);
					if((bytes_red=fread(message,1,bytes_left,fp))<0) perror("last chunk of bytes red failed");
					printf("last bytes red %d\n",bytes_red);
					encrypt_data(message,bytes_red);
					while(1){
						if((bytes_sent=server_send(sock,message,bytes_red,remote,remote_length))<0) perror("Last chunk failed to send");
						printf("last bytes sent %d\n",bytes_sent);
						bzero(buffer,MAXBUFSIZE);
                                                if((nbytes=recvfrom(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *)&remote,&remote_length))<0)     perror("failed to receive ACK for sent packet");
                                                if(nbytes==3){
                                                	printf("Received ACK for sent fragment and will send next packet\n");
                                                        break;
                                                 }
                                                 else printf("Failed to get ACK for sent packet and send again\n");
					}
					fclose(fp);
					printf("File transfer is complete\n");
				//send the md5sum of the file
					server_send(sock,response,sizeof(response),remote,remote_length);
				}
		}
		else if(strcmp(command,com4)==0){
				printf("client is asking to delete %s file\n",filename);
				if(access(filename,F_OK)!=0){
					printf("File doesn't exists\n");
					server_send(sock,"NEGACK",6,remote,remote_length);
				}
				else{
					printf("File exists and deleting the file\n");
					if((remove(filename))==0){
						printf("\nFile is removed\n");
						server_send(sock,"ACK",3,remote,remote_length);
					}
					else {
						printf("\nFile remove failed\n");
						server_send(sock,"NEGACK",3,remote,remote_length);
					}
				}				
		}
		else if (strcmp(command,com5)==0){
				printf("\nClient has asked to shut down\n");
				server_send(sock,"ACK",3,remote,remote_length);
				close(sock);
				exit(1);
		}
		else{
				printf("\nI do not understand what you asking: %s,please give me valid command\n",command);
				server_send(sock,invalid_command,MAXBUFSIZE,remote,remote_length);
		}
	}
	close(sock);
}
