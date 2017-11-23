#include <sys/socket.h>
#include <stddef.h>
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
#include <memory.h>
#include <errno.h>
#include <sys/types.h>
#include <strings.h>

#define MAXBUFSIZE 100
#define KEY 0XAA
//encypting the message with key
void encrypt_data(char *message,int len){
	printf("Encrypting the data\n");
	int i;
        for(i = 0; i < len; i++)
                message[i] ^= KEY;

}

//decrypting the message with key
void decrypt(char *message,int len){
	printf("Decrypting the data\n");
	int i;
        for(i = 0; i <len; i++)
                message[i] ^= KEY;

}

int main (int argc, char * argv[])
{

	int nbytes;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
	char buffer[MAXBUFSIZE];
	char end_buffer[]="ACK";
	struct sockaddr_in remote;              //"Internet socket address structure"
	FILE *fp;
	if (argc < 3)
	{
		printf("\nUSAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	/******************
	  Here we populate a sockaddr_in struct with
	  information regarding where we'd like to send our packet 
	  i.e the Server.
	 ******************/
	bzero(&remote,sizeof(remote));               //zero the struct
	remote.sin_family = AF_INET;                 //address family
	remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address
	int addr_length = sizeof(struct sockaddr);
	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET,SOCK_DGRAM,0)) < 0)
	{
		perror("Unable to create socket\n");
	}
	//We don't want recvfrom stuck forever hence we are making it unblock socket
        struct timeval read_timeout;
        read_timeout.tv_sec = 10;
        read_timeout.tv_usec = 10;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
	/******************
	  sendto() sends immediately.  
	  it will report an error if the message fails to leave the computer
	  however, with UDP, there is no error if the message is lost in the network once it leaves the computer.
	 ******************/
	while(1){
		printf("\nWhat do you want from the running server?\nls\nput <filename>\nget <filename>\ndelete <filenamr>\nexit\n");
		fgets(buffer,MAXBUFSIZE,stdin);
		char * command,message[MAXBUFSIZE],message1[100],message2[100],*filename;
		char put[]="put";
		char get[]="get";
		char ls[]="ls";
		char delete[]="delete";
		char exit[]="exit";
		strcpy(message,buffer);
		strcpy(message1,buffer);
		strcpy(message2,buffer);
		command=strtok(message," \n\t");
		filename=strtok(message1," \t");
        	filename=strtok(NULL,"\n");
		if(strcmp(command,put)==0){
			//send the file to server
                	if((fp=fopen(filename,"rb"))==NULL){
                		perror("Cannot open file it might not exists\n");
				printf("send file that exists\n");
				continue;
			}
			else{
				printf("\nClient has to put a file %sto server\n",filename);
				if((nbytes = sendto(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *)&remote,sizeof(remote)))<0)
	                        	perror("Failed to send packets to server\n");
				//calculating md5sum of the file
				char message_md[100],md5sum_command[100];
				FILE *md5_fp;
				int nbytes,chunk_size=0,bytes_left,bytes_red,chunks;
				char *pack="ACK";
				strcpy(md5sum_command,"md5sum ");
				strcat(md5sum_command,filename);
				if((md5_fp=popen(md5sum_command,"r"))==NULL)	printf("Cannot opend the md5 file\n");
				while(fgets(message_md,100,md5_fp));
				strtok(message_md," ");
				printf("md5sum of the file is %s\n",message_md);
				//calulation of md5 is done

				fseek(fp,0,SEEK_END);
				size_t file_chunk_size=ftell(fp);
				fseek(fp,0,SEEK_SET);
				bzero(message,sizeof(message));
				printf("File chunk_size is %d\n",file_chunk_size);
				if(file_chunk_size > MAXBUFSIZE){
					printf("your file chunk_size is bigger than %d  bytes hence we need to do fragmentation\n",MAXBUFSIZE);
					chunks=file_chunk_size/MAXBUFSIZE;
					printf("total number of chunks %d\n",chunks);
				}
				//send the file chunk_size
				sprintf(message,"%d",file_chunk_size);
				printf("Lets send the file chunk_size\n");
				while(1){
					if(sendto(sock,message,MAXBUFSIZE,0,(struct sockaddr *)&remote,sizeof(remote))<0)
						perror("send to failed for file chunk_size");
					else{
						printf("File chunk_size sent\n");
						bzero(message,MAXBUFSIZE);
						if((nbytes=recvfrom(sock,message,MAXBUFSIZE,0,(struct sockaddr *)&remote,&addr_length))<0) perror("ACK for file size failed\n");
						if(nbytes==3){
							break;
							printf("ACK for file size received\n");}
					}
				}	
				bzero(message,sizeof(message));
				while(chunks--){
					printf("started sending the chunks\n");
					bzero(message,MAXBUFSIZE);
                			if((bytes_red=fread(message,1,MAXBUFSIZE,fp))<0) perror("Failed to send packets to server\n");
					printf("bytes red %d\n",bytes_red);
					encrypt_data(message,bytes_red);		
					while(1){
						if((nbytes = sendto(sock,message,bytes_red,0,(struct sockaddr *)&remote,sizeof(remote)))<0)
							perror("failed to send file accross the server\n");
						bzero(message,MAXBUFSIZE);
						if((nbytes=recvfrom(sock,message,MAXBUFSIZE,0,(struct sockaddr *)&remote,&addr_length))<0) perror("Failed to receive the ACk");
						if(nbytes==3) {
							printf("Received ACK for sent packet and send next packet\n");
							break;
						}
						else printf("Failed to receive ACK for sent packet and send again the packet\n");
					}
					chunk_size+=bytes_red;
					fseek(fp,chunk_size,SEEK_SET);					
				}
				bytes_left=file_chunk_size-chunk_size;
				printf("Lets send the left bytes %d\n",bytes_left);
				if((bytes_red=fread(message,1,bytes_left,fp))<0) perror("last chunk of bytes red failed");
				printf("bytes red %d\n",bytes_red);
				encrypt_data(message,bytes_red);
				while(1){
					if((nbytes=sendto(sock,message,bytes_red,0,(struct sockaddr *)&remote,sizeof(remote)))<0) perror("sending the last chucnk bytes");
					bzero(message,MAXBUFSIZE);
					if((nbytes=recvfrom(sock,message,MAXBUFSIZE,0,(struct sockaddr *)&remote,&addr_length))<0) perror("Failed to receive the ACk");
                                                if(nbytes==3) {
                                                        printf("Received ACK for sent packet and send next packet\n");
                                                        break;
                                                }
                                                else printf("Failed to receive ACK for sent packet and send again the packet\n");
				}
				fclose(fp);
			//sending the file to server is complete client
			//send md5sum to server
                        	if((nbytes = sendto(sock,message_md,MAXBUFSIZE,0,(struct sockaddr *)&remote,sizeof(remote)))<0)
                                	perror("Failed to ACK server that the file doesn't exist to server\n");
				bzero(message,sizeof(message));
			//expect a ACk/NONACK from server
				if((nbytes=recvfrom(sock,message,MAXBUFSIZE,0,(struct sockaddr *)&remote,&addr_length))<0){
					printf("Receive failed\n");
				}
				if(strcmp(message,pack)==0){
					printf("\nFiles sent successful\n");
					}
				else
					printf("\nfile sent failed and please send again\n");
			}
		
		}
		else if(strcmp(command,get)==0){
			int file_size,fragments=0,chunk_size=0,bytes_left,bytes_written;
			if((nbytes = sendto(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *)&remote,sizeof(remote)))<0)
				perror("Failed to send packets to server\n");
			remove(filename);
			//create a file to receive response from server
			if((fp=fopen(filename,"wb+"))==NULL) perror("cannot open file:");
			
			//receive the file size
			bzero(buffer,sizeof(buffer));
			while(1){
				bzero(buffer,MAXBUFSIZE);
				if((nbytes = recvfrom(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *)&remote,&addr_length))<0) perror("Failed to receive file size from server");
				file_size=atoi(buffer);
				printf("file size received is %d\n",file_size);
				if(file_size>0){
					printf("File size is received\n");
					if((nbytes=sendto(sock,"ACK",3,0,(struct sockaddr *)&remote,sizeof(remote)))<0) perror("failed to send ACK for receiving the file size");
					break;
				}
				else{
					printf("File size is not received and hence requesting again\n");
					if((nbytes=sendto(sock,"NEGACK",6,0,(struct sockaddr *)&remote,sizeof(remote)))<0) perror("failed to send ACK for receiving the file size");
				}
			}			
			//read the file  from server and write it to opened file
			fragments=file_size/MAXBUFSIZE;
			printf("total number of fragments to receive from client is %d\n",fragments);
			bzero(buffer,sizeof(buffer));
			fseek(fp,0,SEEK_SET);
			while(fragments--){
				bzero(buffer,MAXBUFSIZE);
				while(1){
					bzero(buffer,MAXBUFSIZE);
					if((nbytes = recvfrom(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *)&remote,&addr_length))<0) perror("Failed to receive from server\n");
					//printf("bytes received from server %d\n",nbytes);
					if(nbytes==MAXBUFSIZE){
						//printf("Received the fragment\n");
						sendto(sock,"ACK",3,0,(struct sockaddr *)&remote,sizeof(remote));
						break;
					}
					else{
						printf("Failed to receive the fragment and hence ask again\n");
						sendto(sock,"NEGACK",6,0,(struct sockaddr *)&remote,sizeof(remote));
					}
				}
				decrypt(buffer,nbytes);
				if((bytes_written=fwrite(buffer,1,nbytes,fp))<0)	perror("Failed to write bytes to file");	
				//printf("bytes written:%d\n",bytes_written);
				chunk_size+=nbytes;
				fseek(fp,chunk_size,SEEK_SET);
			}
			bytes_left=file_size-chunk_size;
			printf("total bytes left to receive %d\n",bytes_left);
			bzero(buffer,MAXBUFSIZE);
			while(1){
				bzero(buffer,MAXBUFSIZE);
				if((nbytes = recvfrom(sock,buffer,bytes_left,0,(struct sockaddr *)&remote,&addr_length))<0) perror("Failed to receive from server\n");
                        	printf("last bytes received from server %d\n",nbytes);
				if(nbytes==bytes_left){
                                                printf("Received the fragment\n");
                                                sendto(sock,"ACK",3,0,(struct sockaddr *)&remote,sizeof(remote));
                                                break;
                                        }
                                        else{
                                                printf("Failed to receive the fragment and hence ask again\n");
                                                sendto(sock,"NEGACK",6,0,(struct sockaddr *)&remote,sizeof(remote));
                                        }
			}
			decrypt(buffer,nbytes);
                        if((bytes_written=fwrite(buffer,1,nbytes,fp))<0)        perror("Failed to write bytes to file");
                        printf("last bytes written:%d\n",bytes_written);
			fclose(fp);
			
			//calculate the md5sum of file received
			char md5sum[100];
                        FILE *fp_md;
			char open_com[50];
			strcpy(open_com,"md5sum ");
			strcat(open_com,filename);
			fp_md=popen(open_com,"r");
                        while(fgets(md5sum,sizeof(md5sum)-1,fp_md)!=NULL);
                        fclose(fp_md);
                        strtok(md5sum," ");//calculation is over

			//receive the md5sum from server
			bzero(buffer,sizeof(buffer));
			if((nbytes = recvfrom(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *)&remote,&addr_length))<0){
                        	printf("\nFailed to receive packet from server\n");}

			//compare the md5sums
			printf("\nReceived md5sum is %s\ncomputed md5sum is %s\n",buffer,md5sum);
			if(strcmp(buffer,md5sum)==0){
				printf("File received is same as sent from server\n");
			}
			else{
				printf("File you requested might not exist and hence ask something that exists\n");
			}
		}
		else if(strcmp(command,ls)==0){
			if((nbytes = sendto(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *)&remote,sizeof(remote)))<0)
                                perror("Failed to send packets to server\n");
			while(1){
				bzero(buffer,sizeof(buffer));
				if((nbytes = recvfrom(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *)&remote,&addr_length))<0){
					printf("\nFailed to receive packet from server\n");}
				if(strcmp(buffer,end_buffer)==0){
					printf("\ngot the response from the server\n");
					break;
				}
				fputs(buffer,stdout);
			}
		}
		else if(strcmp(command,delete)==0){
			if((nbytes = sendto(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *)&remote,sizeof(remote)))<0)
                                perror("Failed to send packets to server\n");
			bzero(buffer,sizeof(buffer));
                        if((nbytes = recvfrom(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *)&remote,&addr_length))<0){
                        	printf("\nFailed to receive packet from server\n");}
			if(strcmp(buffer,end_buffer)==0){
				printf("file deletion is successfull\n");}
			else{
				printf("File deletion failed\n");}
		}
		else if(strcmp(command,exit)==0){
			if((nbytes = sendto(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *)&remote,sizeof(remote)))<0)
                                perror("Failed to send packets to server\n");
			bzero(buffer,sizeof(buffer));
                        if((nbytes = recvfrom(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *)&remote,&addr_length))<0){
                                printf("\nFailed to receive packet from server\n");}
                        if(strcmp(buffer,end_buffer)==0){
                                printf("server ended normally\n");}
                        else{
                                printf("server didn't end normally\n");}
		}
		else{
			if((nbytes = sendto(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *)&remote,sizeof(remote)))<0)
                                perror("Failed to send packets to server\n");
                        bzero(buffer,sizeof(buffer));
                        if((nbytes = recvfrom(sock,buffer,MAXBUFSIZE,0,(struct sockaddr *)&remote,&addr_length))<0){
                                printf("\nFailed to receive packet from server\n");}
			printf("%s\n",buffer);
		}
	}
	close(sock);
}

