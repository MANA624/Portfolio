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
#include <memory.h>
#include <string.h>
#include <stdbool.h>
#include <openssl/md5.h>
/* You will have to modify the program below */

#define MAXBUFSIZE 100
#define MAXFILESIZE 1<<20 // 2^20=1048576

int readMD5(char *fileName, char *data);

int main (int argc, char * argv[] )
{
	int sock;                                                //This will be our socket
	struct sockaddr_in sin, remote;                          //"Internet socket address structure"
	unsigned int remote_length;                              //length of the sockaddr_in structure
	int nbytes;                                              //number of bytes we receive in our message
	bool should_exit = false;                                 // A "boolean" that decides if the program exits or not
	char buffer[MAXBUFSIZE], file_buffer[MAXFILESIZE];       //a buffer to store our received message
	char msg[100] = "";
	char *request_file;
	FILE *fp;
	size_t file_size;
	int i;
	bool cont;
	char recvHash[1024], hash[1024];
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


	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create socket");
	}


	/******************
	  Once we've created a socket, we must bind that socket to the
	  local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		printf("unable to bind socket\n");
	}

	remote_length = sizeof(remote);

	// Waits for an incoming message
	bzero(buffer,sizeof(buffer));
	// Loops through until the user types 'exit' or presses ctrl+c
	printf("Starting server!\n");
	while(!should_exit){
        nbytes = recvfrom(sock, buffer, 1024, 0, (struct sockaddr *)&remote, &remote_length);

        printf("The client says %s\n", buffer);

        strcpy(msg, "");  // Clear the message that the server is going to send back
        if(strncmp(buffer, "get ", strlen("get ")) == 0){
            // "Skip over" the first part of the character array to get the requested file name
            request_file = &buffer[strlen("get")+1];
            fp = fopen(request_file,"rb");
            if(fp==NULL){
                printf("File does not exist\n");
            }
            else{
                // Set the seek to read the file, then read it in, and then send it to the client
                fseek(fp,0,SEEK_END);
                file_size = ftell(fp);
                fseek(fp,0,SEEK_SET);
                fread(file_buffer,file_size,1,fp);
                nbytes = sendto(sock, file_buffer, strlen(file_buffer), 0, (struct sockaddr *)&remote, remote_length);
                bzero(file_buffer,sizeof(file_buffer));
                strcpy(msg, "File successfully sent\n");
            }
        }
        else if(strncmp(buffer, "put ", strlen("put ")) == 0){
            // Zero the file buffer so we're not writing on top of previous sent files
            bzero(file_buffer, sizeof(file_buffer));

            // Wait for the client to send the file
            if(recvfrom(sock, file_buffer, MAXFILESIZE, 0, NULL, NULL)<0){
                printf("error in recieving the file\n");
                exit(1);
            }
            // Copy the contents into a character array, then write them to the appropriate file
            char contents[strlen(file_buffer)];
            strcpy(contents, file_buffer);
            request_file = &buffer[strlen("put")+1];
            fp = fopen(request_file, "wb+");
            if(fwrite(contents, 1, sizeof(contents), fp) < 0){
                printf("Error when writing file. Tough luck\n");
            }
            fclose(fp);
        }

        // This is the functionality for the RELIABLE transfer, which is different from the regular transfer. In the reliable transfer,
        // we first receive the MD5 hash, then we receive the file, then we take the MD5 of the file, and if the two don't match,
        // we don't send an okay, and re-request the file from the client
        else if(strncmp(buffer, "rput ", strlen("rput ")) == 0){
            i = 0;
            cont = false;
            while(!cont && i < 5){
                // Zero the file buffer so we're not writing on top of previous sent files
                bzero(file_buffer, sizeof(file_buffer));

                // Wait for the client to send the md5
                if(recvfrom(sock, recvHash, MAXFILESIZE, 0, NULL, NULL)<0){
                    printf("Error in recieving the MD5 hash\n");
                    exit(1);
                }

                // Wait for the client to send the file
                if(recvfrom(sock, file_buffer, MAXFILESIZE, 0, NULL, NULL)<0){
                    printf("Error in recieving the file\n");
                    exit(1);
                }
                // Copy the contents into a character array, then write them to the appropriate file
                char contents[strlen(file_buffer)];
                strcpy(contents, file_buffer);
                request_file = &buffer[strlen("rput")+1];
                fp = fopen(request_file, "wb+");
                if(fwrite(contents, 1, sizeof(contents), fp) < 0){
                    printf("Error when writing file. Tough luck\n");
                }
                fclose(fp);

                if(readMD5(request_file, hash)){
                    if(strcmp(recvHash, hash) == 0){
                        cont = true;

                        // Send the user the okay!
                        strcpy(msg, "ok");
                        nbytes = strlen(msg) + 1;
                        nbytes = sendto(sock, msg, nbytes, 0, (struct sockaddr *)&remote, remote_length);
                    }
                    else{
                        printf("Hashes not matching\n");
                        // Send the user the bad news
                        strcpy(msg, "nm");  // Not matching
                        nbytes = strlen(msg) + 1;
                        nbytes = sendto(sock, msg, nbytes, 0, (struct sockaddr *)&remote, remote_length);
                    }
                }
                else{
                    printf("Error in calculating hash\n");
                    // Send the user the okay!
                    strcpy(msg, "rt");  // retry
                    nbytes = strlen(msg) + 1;
                    nbytes = sendto(sock, msg, nbytes, 0, (struct sockaddr *)&remote, remote_length);
                }
                i++;
            }
        }

        // If the client wants us to delete one of our files
        else if(strncmp(buffer, "delete", strlen("delete")) == 0){
            request_file = &buffer[strlen("delete")+1];
            if(access(request_file, F_OK ) != -1 ){
                if(remove(request_file) == 0){
                    strcpy(msg, "File deleted!");
                }
                else{
                    strcpy(msg, "File could not be deleted :(");
                }
            }
            else{
                strcpy(msg, "File not found!");
            }
        }

        // If the client wants to see what is in the current directory
        else if(strcmp(buffer, "ls") == 0){
            FILE *fp;
            char var[40];

            // Execute a popen call rather than system() so that we can store the contents of the output
            fp = popen("ls", "r");
            while (fgets(var, sizeof(var), fp) != NULL)
            {
                strcat(msg, var);
            }

            // printf("%s\n", msg); // List contents locally so the server sees what it is sending
            pclose(fp);
        }
        // When we exit the loop
        else if(strcmp(buffer, "exit") == 0){
            printf("Exiting!\n");
            strcpy(msg, "The server is exiting!");
            should_exit = true;
        }
        else{
            strcpy(msg, "Command unknown!");
        }

        // This is the final string status that we send back to the client every iteration
        nbytes = strlen(msg) + 1;
        nbytes = sendto(sock, msg, nbytes, 0, (struct sockaddr *)&remote, remote_length);
    }

	close(sock);

	exit(0);
}

// This reads a file, calculates the MD5 hash, then stores it in *data
int readMD5(char *fileName, char *data) {
    unsigned char c[MD5_DIGEST_LENGTH];
    int i;
    FILE *inFile = fopen (fileName, "rb");
    MD5_CTX mdContext;
    int bytes;

    if (inFile == NULL) {
        printf ("%s can't be opened.\n", fileName);
        return 0;
    }

    // Semantics of using OpenSSL
    MD5_Init (&mdContext);
    while ((bytes = fread (data, 1, 1024, inFile)) != 0){
        MD5_Update (&mdContext, data, bytes);
    }
    // Store the output in c
    MD5_Final (c, &mdContext);

    for(i = 0; i < MD5_DIGEST_LENGTH; i++){
        // Apparently the characters are not normal-sized chars, so we increment data by 2*i for padding
        snprintf(data+(2*i), 1024, "%02x", c[i]);
    }
    printf("\n");
    fclose (inFile);

    return 1;
}

