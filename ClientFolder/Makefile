CC=gcc
RM=rm -f

default: all
all: client
client: udp_client.c
	$(CC) -w -o client udp_client.c
clean:
	$(RM) client
