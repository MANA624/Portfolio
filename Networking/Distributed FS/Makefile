all: DFS DFC

DFS: DFS.c
	gcc -Wall DFS.c -o DFS -lssl -lcrypto
DFC: DFC.c
	gcc -Wall DFC.c -o DFC -lssl -lcrypto

clean:
	$(RM) DFS DFC *.o
