#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>

#define MAXBUFSIZE 1524
#define MAXFILESIZE 1<<19
#define MAXUSERS 10


// char *get_filename_ext(const char *filename);
void intHandler(int sig);
int recvSize(int connfd);
void populateConfig(char confFile[]);
bool checkLogin(char *user, char *pass);
int sendSize(int sock, int len);

char usernames[MAXUSERS][30];
char passwords[MAXUSERS][30];
bool running = true;
int sock;

int main (int argc, char * argv[] )
{
    // Declare all our initial variables
	struct sockaddr_in sin, remote;
	struct stat st;
	struct sigaction sa;
	char buf[MAXFILESIZE];
	char user[30], pass[30];
	socklen_t remote_length = sizeof(remote);
	FILE *fp = NULL;
	char *line = NULL;
	size_t len;
	int on = 1;
    int connfd;
    int messageLen;
    int fdimg;
    int fileSize;
    char command[MAXBUFSIZE];
    char filename[20];
    char usersFolder[20];
    char lsFile[70];
    int requested[4];
    char toSend[4][30];
    int whichFile[4];
    char numHas[2];
    int numWeHave, numRequested;
    int i;
    char symbol[2];

    if(argc < 3){
        printf("USAGE: ./DFS <server_folder> <server_port> &\n");
        exit(1);
    }

    populateConfig("dfs.conf");

    const char *folder = argv[1];

    // Ensure that the directory exists. If it does, rm -rf it. Then create it
    if (opendir(folder))
    {
        /* Directory exists. */
        strcpy(command, "rm -rf ");
        strcat(command, folder);
        system(command);
    }
    mkdir(folder, S_IRWXG | S_IRWXO | S_IRWXU);

    // Pass in functions to struct handler to handle most interrupts
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sa.sa_handler = intHandler;
    sigaction(SIGCHLD, &sa, 0);
    sigaction(SIGINT, &sa, NULL);

	// Info about the address struct
	bzero(&sin,sizeof(sin));                    //zero the struct
	sin.sin_family = AF_INET;                   //address family
	sin.sin_port = htons(atoi(argv[2]));        //htons() sets the port # to network byte order
	sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine

    // Create the socket
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Unable to create socket");
	}
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

    // Bind to the socket
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		printf("Unable to bind socket\n");
		exit(0);
	}

	listen(sock, 10);



	// Start while loop
	while(running){
        // reset sending buffer, accept connection and read request
        memset(buf, 0, MAXBUFSIZE);

        connfd = accept(sock, (struct sockaddr*)&remote, &remote_length);
        bzero(buf, MAXFILESIZE);
        messageLen = recvSize(connfd);

        bzero(buf, MAXFILESIZE);
        read(connfd, buf, messageLen);

        strcpy(user, strtok(buf, " "));
        strcpy(pass, strtok(NULL, " "));
        strcpy(buf, strtok(NULL, "\n"));

        printf("Command: %s\n", buf);

        // Send a response to the user if their credentials were validated
        if(checkLogin(user, pass)){
            write(connfd, "y", sizeof(char));
        }
        else{
            write(connfd, "n", sizeof(char));
            continue;
        }


        if(!fork()){
            // This child will die, so throw its socks away
            close(sock);

            // Check the user's folder and ensure that it exists, creating
            // it if it doesn't
            strcpy(usersFolder, folder);
            strcat(usersFolder, user);
            strcat(usersFolder, "/");

            strcpy(lsFile, usersFolder);
            strcat(lsFile, "ls.txt");

            if(!opendir(usersFolder)){
                mkdir(usersFolder, S_IRWXG | S_IRWXO | S_IRWXU);
            }

            // If the client wants to store a file in their directory. Simple
            if(strncmp(buf, "put ", 4) == 0){
                // Read in the file, then write it a byte at a time (to
                // handle binary files) to the file.
                strcpy(filename, usersFolder);
                strcat(filename, &buf[4]);
                bzero(buf, MAXFILESIZE);
                messageLen = recvSize(connfd);
                read(connfd, buf, messageLen);

                fp = fopen(filename, "wb");
                if(fp){
                    int i;
                    for(i=0; i<messageLen; i++){
                        fwrite(&buf[i], 1, 1, fp);
                    }
                }
                else{
                    printf("Could not open file for writing\n");
                }
                fclose(fp);
                remove(lsFile);

            }
            // If the client wants to retrieve a file
            else if(strncmp(buf, "get ", 4) == 0){
                // Clear out all of our variables
                for(i=0; i<4; i++){
                    requested[i] = 0;
                    whichFile[i] = 0;
                }
                // Read in which partitions they need based on the request
                // Start at the end, store a byte as an int, then delete it
                i = 0;
                while(buf[strlen(buf)-1] != ' '){
                    requested[i] = buf[strlen(buf)-1]-'0';
                    i++;
                    buf[strlen(buf)-1] = 0;
                }
                numRequested = i;
                buf[strlen(buf)-1] = 0;

                strcpy(buf, &buf[4]);
                strcpy(filename, usersFolder);
                strcat(filename, buf);

                // Carried out by using ls system command, then reading in and deleting that file
                strcpy(command, "ls ");
                strcat(command, usersFolder);
                strcat(command, " > ");
                strcat(command, lsFile);
                system(command);
                if(stat(lsFile, &st) != 0){
                    printf("Could not read in file!\n");
                    close(connfd);
                    exit(0);
                }

                // Go through all files in the directory, and if they match the requested file,
                // Keep track of it to be sent to the user later. Then remove the lsFile
                fp = fopen(lsFile, "r");
                numWeHave = 0;
                while(getline(&line, &len, fp) != -1){
                    line[strlen(line)-1] = 0;
                    //printf("%s\n", line);
                    if((strlen(line)-2 == strlen(buf)) && (strncmp(buf, line, strlen(buf)) == 0)){
                        for(i=0; i<numRequested; i++){
                            if(requested[i] == line[strlen(line)-1]-'0'){
                                strcpy(toSend[numWeHave], usersFolder);
                                strcat(toSend[numWeHave], line);
                                whichFile[numWeHave] = requested[i];
                                numWeHave++;
                            }
                        }
                    }
                }
                remove(lsFile);

                // Inform the user how many requested partitions we have
                sprintf(numHas, "%d", numWeHave);
                write(connfd, numHas, 1);

                // Send all of them over
                for(i=0; i<numWeHave; i++){
                    printf("File: %s\n", toSend[i]);
                    if(stat(toSend[i], &st) == 0){
                        fileSize = st.st_size;
                        sendSize(connfd, fileSize);
                        sendSize(connfd, whichFile[i]);

                        // Go through byte-by-byte, store the char, and then
                        // send the buffer when we are done
                        /*
                        bzero(buf, MAXFILESIZE);

                        FILE *fp = fopen(toSend[i], "rb");
                        if(fp != NULL)
                        {
                            while((symbol[0] = getc(fp)) != EOF)
                            {
                                strcat(buf, symbol);
                            }
                            fclose(fp);
                        }
                        write(connfd, buf, fileSize);
                        */
                        fdimg = open(toSend[i], O_RDONLY); //
                        sendfile(connfd, fdimg, 0, fileSize);
                    }
                }

            }
            // The client wants to know which files we have. Run system ls
            // and send the results to them, then remove the file
            else if(strcmp(buf, "ls") == 0){
                strcpy(command, "ls ");
                strcat(command, usersFolder);
                strcat(command, " > ");
                strcat(command, lsFile);
                system(command);
                if(stat(lsFile, &st) == 0){
                    fdimg = open(lsFile, O_RDONLY);
                    fileSize = st.st_size;
                    sendSize(connfd, fileSize);
                    sendfile(connfd, fdimg, NULL, fileSize);
                    remove(lsFile);
                }
                else{
                    printf("Could not read in file!\n");
                }
            }
            else{
                printf("Command not recognized!\n");
            }

            close(connfd);
            exit(0);
        }

        close(connfd);
    }

    close(connfd);
	exit(0);
}

// Go through valid usernames and passwords and check
// if the ones that the user send are in there
bool checkLogin(char *user, char *pass){
    int i;
    for(i=0; i<MAXUSERS; i++){
        if(strcmp(user, usernames[i]) == 0){
            if(strcmp(pass, passwords[i]) == 0){
                return true;
            }
            else{
                return false;
            }
        }
    }
    return false;
}

// Get the extension of the filename. Not needed
/*
char *get_filename_ext(const char *filename) {
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}
*/

// The same recvSize function in the client's file
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
    }while(buf[bufLength-1] != ' ' && bufLength < 15);
    buf[bufLength-1] = 0;
    return atoi(buf);
}

// The same sendSize function in the client's file
int sendSize(int sock, int len){
    char cmdLen[20];
    bzero(cmdLen, 20);

    sprintf(cmdLen, "%d", len);
    write(sock, cmdLen, strlen(cmdLen)*sizeof(char));

    write(sock, " ", sizeof(char));

    return 1;
}


// Reads in the config file and stores all the values in the cf struct. Ez
void populateConfig(char confFile[]){
    FILE *fp;
    char string1[1000], string2[1000];
    char *line = NULL;
    size_t len = 0;
    int name = 0;

    fp = fopen(confFile, "r");

    while(fscanf(fp, "%s", string1) != -1){
        // If the line is a comment
        if(string1[0] == '#'){
            getline(&line, &len, fp);
            continue;
        }

        else{
            // If not a comment, read in second value
            fscanf(fp, "%s", string2);
            strcpy(usernames[name], string1);
            strcpy(passwords[name], string2);
            name++;
        }
    }
    free(line);
    fclose(fp);
}



// This is the function that reaps zombie processes and causes a clean exit and frees resources
// It handles most interrupts in general
void intHandler(int sig){
    // The signal is two when the user enters ctrl+c
    if(sig == 2){
        close(sock);
        exit(0);
    }
    // When the child status has changed
    else if(sig == 17){
        int saved_errno = errno;
        while (waitpid((pid_t)(-1), 0, WNOHANG) > 0);
        errno = saved_errno;
    }
}

