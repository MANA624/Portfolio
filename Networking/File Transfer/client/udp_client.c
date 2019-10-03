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
#include <memory.h>
#include <errno.h>
#include <stdbool.h>
#include <openssl/md5.h>

#define MAXBUFSIZE 2048
#define MAXFILESIZE 1<<20 // 2^20=1048576

/* You will have to modify the program below */

int readMD5(char *fileName, char *data);

int main (int argc, char * argv[])
{
    printf("%d\n", MAXFILESIZE);
	int nbytes;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
	char buffer[MAXBUFSIZE], file_buffer[MAXFILESIZE], send_file[MAXFILESIZE];
	char *request_file;
	bool should_exit = false;
	socklen_t addr_size;
	FILE *fp;
	size_t file_size;
	int i = 0;
	char hash[1024];

	struct sockaddr_in remote;              //"Internet socket address structure"

	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
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

	addr_size = sizeof remote;

	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create socket");
	}

	/******************
	  sendto() sends immediately.
	  it will report an error if the message fails to leave the computer
	  however, with UDP, there is no error if the message is lost in the network once it leaves the computer.
	 ******************/
    while(!should_exit){
        printf("Please enter a command:");
        fgets(buffer, MAXBUFSIZE, stdin);
        buffer[strlen(buffer)-1] = 0; // Take out the newline character before sending
        printf("You entered: %s\n", buffer);
        nbytes = strlen(buffer) + 1;

        nbytes = sendto(sock, buffer, nbytes, 0, (struct sockaddr *)&remote, addr_size);

        if(strcmp(buffer, "exit") == 0){
            should_exit = true;
        }
        else if(strncmp(buffer, "get ", strlen("get ")) == 0){
            bzero(file_buffer, sizeof(file_buffer));
            // bzero(requested_file, sizeof(requested_file));

            if(recvfrom(sock, file_buffer, MAXFILESIZE, 0, NULL, NULL)<0){
              printf("error in recieving the file\n");
              exit(1);
            }
            char contents[strlen(file_buffer)];
            strcpy(contents, file_buffer);
            printf("The file is: %s\n", contents);
            request_file = &buffer[strlen("get")+1];
            fp = fopen(request_file, "w+");
            if(fwrite(contents, 1, sizeof(contents), fp) < 0){
                printf("Error when writing file. Tough luck\n");
            }
            fclose(fp);
        }
        else if(strncmp(buffer, "put ", strlen("put ")) == 0){
            request_file = &buffer[strlen("put")+1];
            fp = fopen(request_file, "rb");
            if(fp==NULL){
                printf("File does not exist\n");
            }
            else{
                // First go to the end of the file, then ge the file size, then
                fseek(fp, 0, SEEK_END);
                file_size = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                printf("Bytes read: %zu\n", fread(file_buffer, file_size, 1, fp));
                printf("File size: %zu\n", file_size);
                printf("%s\n", file_buffer);
                i = 0;
                while (i < file_size){
                   // printf("%c",((unsigned char)file_buffer[i]));
                   send_file[i] = ((unsigned char)(file_buffer[i]));
                   printf("%c", send_file[i]);
                   i++;
                   //if( ! (i % 16) ) printf( "\n" );
                }
                printf("Hey: %.*s", 1070, send_file);
                printf("Size: %zu\n", strlen(send_file));
                nbytes = sendto(sock, file_buffer, strlen(file_buffer), 0, (struct sockaddr *)&remote, addr_size);
                bzero(file_buffer,sizeof(file_buffer));
                //strcpy(msg, "Successfully sent file\n");
            }
        }
        else if(strncmp(buffer, "rput ", strlen("rput ")) == 0){
            request_file = &buffer[strlen("rput")+1];
            printf("File to reed: %s\n", request_file);
            if(!readMD5(request_file, hash)){
                printf("Hash-generating function not working!\n");
            }
            fp = fopen(request_file, "r");
            if(fp == NULL){
                printf("File does not exist\n");
            }
            else{
                // Go to the end of the file, get the file size, then go back to the beginning and read it in
                fseek(fp,0,SEEK_END);
                file_size = ftell(fp);
                fseek(fp,0,SEEK_SET);
                fread(file_buffer,file_size,1,fp);
                i = 0;
                bzero(buffer, sizeof(buffer));

                // While we don't have the okay, first send the hash, then send the file contents. The server will compare
                // the two, and while they are not equal, they will send a bad response
                while(strcmp(buffer, "ok") != 0 && i < 5){
                    nbytes = sendto(sock, hash, strlen(hash), 0, (struct sockaddr *)&remote, addr_size);
                    nbytes = sendto(sock, file_buffer, strlen(file_buffer), 0, (struct sockaddr *)&remote, addr_size);

                    bzero(buffer, sizeof(buffer));
                    nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, NULL, NULL);

                    i++;
                }
                if(i == 5){printf("Error!!! File not sent");}

            }
        }

        bzero(buffer, sizeof(buffer));
        nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, NULL, NULL);

        printf("Server says: %s\n", buffer);
    }

	close(sock);

    exit(0);
}

// Reads the MD5 hash and writes the contents to *data
int readMD5(char *fileName, char *data) {
    unsigned char c[MD5_DIGEST_LENGTH];
    int i;
    FILE *inFile = fopen (fileName, "rb");
    MD5_CTX mdContext;
    int bytes;
    // unsigned char data[1024];

    if (inFile == NULL) {
        printf ("%s can't be opened.\n", fileName);
        return 0;
    }

    MD5_Init (&mdContext);
    while ((bytes = fread (data, 1, 1024, inFile)) != 0){
        MD5_Update (&mdContext, data, bytes);
    }

    // Write contents to the array c
    MD5_Final (c, &mdContext);

    for(i = 0; i < MD5_DIGEST_LENGTH; i++){
        snprintf(data+(2*i), 1024, "%02x", c[i]);
    }
    printf("\n");
    fclose (inFile);

    return 1;
}
