UDP Reliable Protocol with Encryption and verification.

This project has the following directory

1.ClientFolder
	a.udp_client.c
	b.Makefile
2.ServerFolder
	a.udp_server.c
	b.Makefile
3.README.txt

####	Steps to Compile and Run ####

1. Goto ClientFolder and do make clean all,you will see executable called client
2. Goto ServerFolder and do make clean all,you will see executable called server
3. Remote login to server and copy the server executable onto server
4. Know the ip address of the server using uname -i
5. Run the server "./server 5002"
6. On the local machine run the client as "./client <ip address obtained in 4> 5002"
7. choose the option client prompt to user
8. kill server by issueing exit command and kill running client using Ctrl-C

#####################################

Assumptions made:

1. Entire file to send or receive is divided into frames of 100(MAXBUFSIZE) bytes for reliable operation.
2. Hence given file is divided into sizeof(file)/MAXBUFSIZE fragments.
3. when fragment is sent the sender will wait for ACK from reciever befor it sends the next fragments. If ACK is not received it will send the    fragment again till it receives ACK. Hence if the fragment is dropped in the middle it will be received at server by resending it. If the fr   -agment sent is corrupted,sender will send the fragment again. If the ACK from receiver is dropped sender will send the fragment again. If 	the fragment is lost in the middle receiver will wait till it gets the fragment from the sender.But timer is set to 10 S using setsockopt so   that receiver wi   -ll not go to dead state. Hence the reliability is acheived. 
   Hence used protocol is STOP AND WAIT.
4. Encryption is done using simple XOR of each bytes and Decryption is done using the same XOR operation again. 
5. md5sum is used to verify if the file received is same as sent from sender.
6. If client is sending file then sender is the "client" and sever will be the "receiver" and vice-versa.
7. Reliability is achived even when file size is greater than 100MB.

######################################


NOTE: whenever requesting the file or sending the file, filename should be the local filename in the given local repository, program will not work for filename present in other directory/pathname for file is given

