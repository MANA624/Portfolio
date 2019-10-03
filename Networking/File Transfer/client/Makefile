all: client

client: udp_client.c
	gcc -Wall udp_client.c -o client -lssl -lcrypto

clean:
	$(RM) client
