#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
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
#include <string.h>
#include <dirent.h>

#define MAXBUFSIZE 2548
#define MAXFILESIZE 1<<21
#define MAXSHARDSIZE 1<<19
#define MAXUSERS 10
#define NUMFILES 50
#define DOENCRYPT 0

char username[30];
char password[30];
struct sockaddr_in server[4];
char *fileNames[4][NUMFILES];

int connectToSock(char *command, int serv);
void populateConfig(char confFile[]);
int readMD5(char *fileName, char *data);
int sendSize(int sock, int len);
int getStart(int fileSize, int pos);
void insertName(char *filename, int place);
int recvSize(int connfd);

/*
    * This is the client side of the Distributed File System
    * This is where the user will input commands to be processed
    * and sent off to the server in order to store their files
*/
int main (int argc, char* argv[])
{
    // Initializing all variables
    struct stat st;
	int sock;
	int fileSize;
	char command[MAXBUFSIZE], withUsername[MAXBUFSIZE];
	bool should_exit = false;
	char *filename = NULL;
	FILE *fp = NULL;
	char partitionCmd[MAXBUFSIZE];
	int partitionSize;
	int i, j;
	char strI[2];
	char hash[1024];
	char buf[MAXFILESIZE];
	int intHash;
	int fdimg;
	int partition;
	char *token = NULL;
	int partitionNum;
	int fileCounter;
	bool hasShard[4];
	char shards[4][MAXSHARDSIZE];
	int lengths[4];
	int numCollected, numServerHas;
	int numBytes;
	int receiving;
	int cmdLen;
	int cipherLoc;
	char downloadTo[100];
	char symbol[2];

	if (argc < 2)
	{
		printf("USAGE:  DFC <config_file>\n");
		exit(1);
	}

	populateConfig(argv[1]);

	// Ensure that the downloads directory exists. If it does, rm -rf it. Then create it
	const char *downloadFolder = "downloads/";
    if (!opendir(downloadFolder))
    {
        mkdir(downloadFolder, S_IRWXG | S_IRWXO | S_IRWXU);
    }

    // The main loop that won't exit until the user types "exit"
    while(!should_exit){
        printf("Please enter a command:");
        // Clear all buffers to avoid overwriting data
        bzero(command, MAXBUFSIZE);
        bzero(withUsername, MAXBUFSIZE);
        bzero(buf, MAXFILESIZE);
        bzero(partitionCmd, MAXBUFSIZE);
        bzero(downloadTo, 100);
        for(i=0; i<4; i++){
            bzero(shards[i], MAXFILESIZE/4);
        }
        // Get user input and take out newline character
        fgets(command, MAXBUFSIZE, stdin);
        command[strlen(command)-1] = 0;

        // When the user wants to break up a file and send it to the servers
        if(strncmp(command, "put ", 4) == 0){
            filename = &command[4];

            // If we can open the file
            if(stat(filename, &st) == 0){
                // Get MD5 hash to determine where file is stored
                readMD5(filename, hash);

                intHash = (int)strtol(&hash[30], NULL, 16);
                intHash %= 4;
                // Iterate through all servers
                for(i=0; i<4; i++){
                    // Send two file chunks to each server for redundancy
                    for(j=0; j<2; j++){
                        // Append the partition number to the send command
                        partition = (4 - intHash + i + j) % 4;
                        strcpy(partitionCmd, command);
                        strcat(partitionCmd, ".");
                        sprintf(strI, "%d", partition+1);
                        strcat(partitionCmd, strI);

                        strcpy(withUsername, username);
                        strcat(withUsername, " ");
                        strcat(withUsername, password);
                        strcat(withUsername, " ");
                        strcat(withUsername, partitionCmd);

                        sock = connectToSock(withUsername, i);
                        if(sock == -1) break;
                        write(sock, withUsername, strlen(withUsername));

                        // Get confirmation response from server
                        read(sock, buf, 1);
                        if(buf[0] == 'n'){
                            printf("Not a valid Username and Password combo\n");
                            break;
                        }
                        else if(buf[0] == 'y'){
                            fileSize = st.st_size;
                            off_t off = getStart(fileSize, partition);
                            partitionSize = getStart(fileSize, partition+1) - off;

                            sendSize(sock, partitionSize);

                            fdimg = open(filename, O_RDONLY); //
                            sendfile(sock, fdimg, &off, partitionSize); //
                            bzero(buf, MAXFILESIZE);

                            /*
                            FILE *fp = fopen(filename, "rb");
                            if(fp != NULL)
                            {
                                cipherLoc = 0;
                                while((symbol[0] = getc(fp)) != EOF)
                                {
                                    // The following lines are for encryption
                                    if(DOENCRYPT){
                                        symbol[0] = symbol[0] ^ password[cipherLoc];
                                        cipherLoc = (cipherLoc+1) % strlen(password);
                                    }
                                    strcat(buf, symbol);
                                }
                                fclose(fp);
                            }
                            write(sock, &buf[off], partitionSize);
                            */

                        }
                    }
                }
            }
            else{
                printf("File not found!\n");
                continue;
            }
        }

        // When the user wants to retrieve a file from the server
        // DOESN'T WORK FOR LARGE FILES OR BINARY FILES
        else if(strncmp(command, "get ", 4) == 0){
            // Reset all variables
            filename = &command[4];
            for(i=0; i<4; i++){
                hasShard[i] = false;
            }

            for(i=0; i<4; i++){
                bzero(withUsername, MAXBUFSIZE);
                strcpy(withUsername, username);
                strcat(withUsername, " ");
                strcat(withUsername, password);
                strcat(withUsername, " ");
                strcat(withUsername, command);
                strcat(withUsername, " ");
                cmdLen = strlen(withUsername);

                // Check which partition chunks we need to request
                numCollected = 4;
                for(j=0; j<4; j++){
                    if(!hasShard[j]){
                        withUsername[strlen(withUsername)] = j+'1';
                        numCollected--;
                    }
                }
                if(numCollected == 4){
                    break;
                }

                // Request validation with username and password
                sock = connectToSock(withUsername, i);
                if(sock == -1) continue;
                write(sock, withUsername, strlen(withUsername));
                withUsername[cmdLen] = 0;

                read(sock, buf, 1);
                if(buf[0] == 'n'){
                    printf("Not a valid Username and Password combo\n");
                    continue;
                }
                else if(buf[0] == 'y'){
                    // Read in how many needed pieces the server has and remove newline
                    read(sock, buf, 1);
                    numServerHas = buf[0] - '0';
                    // Read in the size of the file, the partition we are receiving, and the file itself
                    for(j=0; j<numServerHas; j++){
                        numBytes = recvSize(sock);
                        receiving = recvSize(sock) - 1;
                        read(sock, shards[receiving], numBytes);

                        lengths[receiving] = numBytes;
                        hasShard[receiving] = true;
                    }
                }
            }
            // Ensure we have all needed shards
            numCollected = 0;
            for(i=0; i<4; i++){
                if(hasShard[i]){
                    numCollected++;
                }
            }
            if(numCollected == 4){
                // Save in downloads/ folder
                strcpy(downloadTo, downloadFolder);
                strcat(downloadTo, filename);
                fp = fopen(downloadTo, "wb");
                if(fp){
                    cipherLoc = 0;
                    for(i=0; i<4; i++){
                        // Write all four shards and decrypt them using the password
                        for(j=0; j<lengths[i]/sizeof(char); j++){
                            if(DOENCRYPT){
                                shards[i][j] = shards[i][j] ^ password[cipherLoc];
                                cipherLoc = (cipherLoc+1) % strlen(password);
                            }
                            fwrite(&shards[i][j], 1, 1, fp);
                        }
                    }

                }
                else{
                    printf("Could not open file for writing\n");
                }
                fclose(fp);
            }
            else{
                printf("File is incomplete\n");
            }
        }

        // Show which files the server currently has in my directory
        else if(strcmp(command, "ls") == 0){
            // Reset all variables
            strcpy(withUsername, username);
            strcat(withUsername, " ");
            strcat(withUsername, password);
            strcat(withUsername, " ");
            strcat(withUsername, command);
            for(i=0; i<4; i++){
                for(j=0; j<NUMFILES; j++){
                    fileNames[i][j] = NULL;
                }
            }
            for(i=0; i<4; i++){
                // Ask all four servers to send an ls of our dir
                bzero(buf, MAXFILESIZE);
                sock = connectToSock(withUsername, i);
                if(sock == -1){
                    continue;
                }
                write(sock, withUsername, strlen(withUsername));
                read(sock, buf, 1);
                if(buf[0] == 'n'){
                    printf("Not a valid Username and Password combo\n");
                    continue;
                }
                else if(buf[0] == 'y'){
                    numBytes = recvSize(sock);
                    read(sock, buf, numBytes);
                    token = strtok(buf, "\n");

                    while(token != NULL){
                        if(strcmp(token, "ls.txt") != 0){
                            partitionNum = token[strlen(token)-1] - '0' - 1;
                            token[strlen(token)-2] = 0;

                            insertName(token, partitionNum);
                        }

                        token = strtok(NULL, "\n");
                    }
                }
            }

            for(i=0; i<NUMFILES; i++){
                fileCounter = 0;
                for(j=0; j<4; j++){
                    if(fileNames[j][i] != NULL){
                        fileCounter++;
                        filename = fileNames[j][i];
                    }
                }
                if(fileCounter == 0){
                    break;
                }
                else if(fileCounter == 4){
                    printf("File: %s\n", filename);
                }
                else{
                    printf("File: %s [incomplete]\n", filename);
                }
            }

        }

        else if(strcmp(command, "exit") == 0){
            should_exit = true;
        }
        else{
            printf("Command not recognized!\n");
        }
    }

	close(sock);

    exit(0);
}

// This is the logic behind the ls file. You take a name and which partition it is
// then go through the fileNames[][] array until there is either a NULL row or
// a row where the file we have belongs. Then store the file in the appropriate place
void insertName(char *filename, int place){
    int i, j;
    bool exists;
    for(i=0; i<NUMFILES; i++){
        exists = false;
        for(j=0; j<4; j++){
            if(fileNames[j][i] != NULL){
                exists = true;
                if(strcmp(filename, fileNames[j][i]) == 0){
                    fileNames[place][i] = strdup(filename);
                    return;
                }
            }
        }
        if(!exists){
            fileNames[place][i] = filename;
            return;
        }
    }
}

// Reads in the config file and stores all the values in the cf struct
// This is very simple and straightforward and works well
void populateConfig(char confFile[]){
    FILE *fp;
    char string1[1000], string2[1000], string3[1000], string4[1000];
    char *line = NULL;
    size_t len = 0;
    int index = 0;

    fp = fopen(confFile, "r");

    while(fscanf(fp, "%s", string1) != -1){
        // If the line is a comment
        if(string1[0] == '#'){
            getline(&line, &len, fp);
            continue;
        }
        // If not a comment, read in second value
        fscanf(fp, "%s", string2);

        if(strcmp(string1, "Server") == 0){
            fscanf(fp, "%s", string3);
            strcpy(string3, strtok(string3, ":"));
            strcpy(string4, strtok(NULL, ":"));

            bzero(&server[index], sizeof(server[index]));       //zero the struct
            server[index].sin_family = AF_INET;                 //address family
            server[index].sin_port = htons(atoi(string4));      //sets port to network byte order
            server[index].sin_addr.s_addr = inet_addr(string3); //sets remote IP address
            index++;
        }

        else if(strcmp(string1, "Username:") == 0){
            strcpy(username, string2);
        }
        else if(strcmp(string1, "Password:") == 0){
            strcpy(password, string2);
        }


    }
    printf("Username: %s\n", username);
    printf("Password: %s\n", password);
    free(line);
    fclose(fp);
}

// Ask the server to send over the number of bytes that it will send you
// in the file it is about to send. Then we can read() the appropriate number
int recvSize(int connfd){
    int bufLength = 0;
    char buf[MAXBUFSIZE];
    bzero(buf, MAXBUFSIZE);

    do{
        if(read(connfd, &buf[bufLength], sizeof(char)) == -1){
            printf("Read error!\n");
            break;
        }
        bufLength++;
    // The server will send a ' ' character when it is done with numbers
    }while(buf[bufLength-1] != ' ' && bufLength < 15);
    buf[bufLength-1] = 0;
    return atoi(buf);
}

// Gets the starting position for the split file based on the partition number
// A simple recursive algorithm. I can't really explain why it works, but it does. Very well
// It accounts for all cases of x where x = md5sum(file) % 4 consistently
int getStart(int fileSize, int pos){
    int rem = fileSize % 4;
    if(pos == 0){
        return 0;
    }

    if(rem > pos-1){
        return getStart(fileSize, pos-1) + (fileSize+4-rem)/4;
    }
    else{
        return getStart(fileSize, pos-1) + fileSize/4;
    }

}

// Create a socket, connect to the server, and send the size of the command it is
// about to send
int connectToSock(char *command, int serv){
    int sock, on = 1;
    socklen_t addr_size = sizeof(server[serv]);

    // Causes the system to create a generic socket of type TCP
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("unable to create socket");
        return -1;
    }
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) < 0){
        printf("setsockopt() failed!\n");
        return -1;
    }

    if(connect(sock, (struct sockaddr *) &server[serv], addr_size) < 0){
        printf("Connection failed!\n");
        return -1;
    }

    sendSize(sock, strlen(command)*sizeof(char));

    return sock;
}

// Send the size of the file that we are about to send over
// Simply store the int as a char[] and then send it with a space afterwards
int sendSize(int sock, int len){
    char cmdLen[20];
    bzero(cmdLen, 20);

    sprintf(cmdLen, "%d", len);
    write(sock, cmdLen, strlen(cmdLen)*sizeof(char));

    write(sock, " ", sizeof(char));

    return 1;
}

// Reads the MD5 hash and writes the contents to *data
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

    MD5_Init (&mdContext);
    while ((bytes = fread (data, 1, 1024, inFile)) != 0){
        MD5_Update (&mdContext, data, bytes);
    }

    // Write contents to the array c
    MD5_Final (c, &mdContext);

    for(i = 0; i < MD5_DIGEST_LENGTH; i++){
        snprintf(data+(2*i), 1024, "%02x", c[i]);
    }
    fclose (inFile);

    return 1;
}
