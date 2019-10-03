all: webproxy

webproxy: webproxy.c
	gcc -Wall webproxy.c -o webproxy -lssl -lcrypto

clean:
	$(RM) webproxy *.o
