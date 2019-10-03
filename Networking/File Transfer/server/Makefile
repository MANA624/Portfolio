all: server

server: udp_server.c
	gcc -Wall udp_server.c -o server -lssl -lcrypto

clean:
	$(RM) server
