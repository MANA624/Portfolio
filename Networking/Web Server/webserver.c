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
/* You will have to modify the program below */

#define MAXBUFSIZE 1024

struct Configuration{
    int port;
    char docRoot[100];
    char dirIndex[20];
    char loc404[30];
    char loc501[30];
    char html[12];
    char htm[12];
    char txt[12];
    char png[12];
    char gif[12];
    char jpg[12];
    char css[12];
    char js[12];
    char ico[12];
} cf;

const char *getContentType(char suffix[]);
void populateConfig();
char *get_filename_ext(const char *filename);
void intHandler(int sig);
char *makePost(char *data, char *file, int pid);

char header[] = "HTTP/1.1 200 OK\r\nContent-Type: ";
char header2[] = "HTTP/1.1 501 Not Implemented\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n";
char header3[] = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n";
bool running = true;
int sock;

int main (int argc, char * argv[] )
{
	struct sockaddr_in sin, remote;             //"Internet socket address structure"
	struct stat st;                             // Get stats on files
	struct sigaction sa;                        // Struct to handle zombie processes
	char buf[MAXBUFSIZE];   //a buffer to store our received message
	char *tempBuf;
	char *protocol, *path;
	char *tempFile;
	char sendHeader[150];
	socklen_t remote_length = sizeof(remote);   //length of the sockaddr_in structure
	int on = 1;
	int fdimg;
    int connfd;
    int i;
	populateConfig();

    // Pass in functions to struct handler to handle all interrupts
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sa.sa_handler = intHandler;
    sigaction(SIGCHLD, &sa, 0);
    sigaction(SIGINT, &sa, NULL);

	// Info about the address struct
	bzero(&sin,sizeof(sin));                    //zero the struct
	sin.sin_family = AF_INET;                   //address family
	sin.sin_port = htons(cf.port);                 //htons() sets the port # to network byte order
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

        if(!fork()){
            // This child will die, so throw its socks away
            close(sock);

            read(connfd, buf, MAXBUFSIZE-1);
            tempBuf = buf;

            protocol = strtok(buf, " ");

            // Test the REST method being used
            if(protocol == NULL);  // Sometimes there are NULL requests and idk why
            else if(strcmp(protocol, "GET") == 0){
                // printf("Protocol: %s\n", protocol);

                path = strtok(NULL, " ");
                path += 1;
                if(strcmp(path, "") == 0){
                    if(stat(cf.dirIndex, &st) == 0){
                        fdimg = open(cf.dirIndex, O_RDONLY);
                        strcpy(sendHeader, header);
                        strcat(sendHeader, getContentType(get_filename_ext(cf.dirIndex)));
                        write(connfd, sendHeader, strlen(sendHeader));
                        sendfile(connfd, fdimg, NULL, st.st_size);
                    }
                    else{
                        // The file could not be read, so send the 404 error
                        // This means that the config file is wrong
                        stat(cf.loc404, &st);
                        write(connfd, header3, sizeof(header3) - 1);
                        fdimg = open(cf.loc404, O_RDONLY);
                        printf("Config file referencing file that doesn't exist!\n");
                        sendfile(connfd, fdimg, NULL, st.st_size);
                    }
                }
                else{
                    // Read is statistics for the file to 1) see if it exists, and
                    // 2) Get the size of the file in bytes
                    if(stat(path, &st) == 0){
                        fdimg = open(path, O_RDONLY);
                        strcpy(sendHeader, header);
                        strcat(sendHeader, getContentType(get_filename_ext(path)));
                        write(connfd, sendHeader, strlen(sendHeader));
                    }
                    else{
                        // The file could not be read, so send the 404 error
                        stat(cf.loc404, &st);
                        write(connfd, header3, sizeof(header3) - 1);
                        fdimg = open(cf.loc404, O_RDONLY);
                        printf("404 not found\n");
                    }
                    sendfile(connfd, fdimg, NULL, st.st_size);
                }

                close(fdimg);

                close(connfd);
            }
            // Extra credit support for POST protocol!
            else if(strcmp(protocol, "POST") == 0){
                path = strtok(NULL, " ");
                path += 1;

                i = 0;
                while(strncmp(tempBuf + i, "\r\n\r\n", 4) != 0) i++;
                i += 4;

                if(strcmp(path, "") == 0){
                    tempFile = makePost(tempBuf+i, cf.dirIndex, getpid());
                }
                else{
                    tempFile = makePost(tempBuf+i, path, getpid());
                }
                stat(tempFile, &st);
                fdimg = open(tempFile, O_RDONLY);
                strcpy(sendHeader, header);
                strcat(sendHeader, getContentType(get_filename_ext(tempFile)));
                write(connfd, sendHeader, strlen(sendHeader));
                sendfile(connfd, fdimg, NULL, st.st_size);
                remove(tempFile);
            }
            // This runs if the protocol isn't GET or POST or something else unsupported
            else{
                printf("Not supported protocol: %s\n", protocol);
                stat(cf.loc501, &st);
                write(connfd, header2, sizeof(header2) - 1);
                fdimg = open(cf.loc501, O_RDONLY);
                sendfile(connfd, fdimg, NULL, st.st_size);
            }

            close(connfd);
            exit(0);
        }
        close(connfd);
    }

	exit(0);
}

// This takes a file, and inserts the POST data into it, returning the name of the used temp file
char *makePost(char *data, char *file, int pid){
    FILE *fp, *tempFP;
    char *line = NULL;
    char *subs = "<body>";
    char *statement1 = "<pre><h1>", *statement2 = "</h1></pre>";
    char firstPart[100], lastPart[100];
    size_t len = 0;
    char *ptr;
    int pos;
    char TFN[15];
    char *ret;
    char pidStr[6];
    sprintf(pidStr, "%d", pid);
    strcpy(TFN, "temp");
    strcat(TFN, pidStr);
    strcat(TFN, ".html");

    fp = fopen(file, "r");

    tempFP = fopen(TFN, "w+");

    while(getline(&line, &len, fp) != -1){
        ptr=strstr(line,subs);

        if(ptr != NULL){
            pos = ptr - line + strlen(subs);
            strcpy(firstPart, line);
            strcpy(lastPart, firstPart);
            firstPart[pos] = 0;

            fputs(firstPart, tempFP);
            fputs(statement1, tempFP);
            fputs(data, tempFP);
            fputs(statement2, tempFP);
            fputs(line+pos, tempFP);
        }
        else{
            fputs(line, tempFP);
        }
    }

    fclose(fp);
    fclose(tempFP);
    free(line);

    ret = TFN;

    return ret;
}

char *get_filename_ext(const char *filename) {
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

// Pass in the suffix to the requested file and get back the appropriate finish to the Content-Type tag
const char *getContentType(char suffix[]){
    if(strcmp(suffix, "html") == 0) return strcat(cf.html, "\r\n\r\n");
    else if(strcmp(suffix, "htm") == 0) return strcat(cf.htm, "\r\n\r\n");
    else if(strcmp(suffix, "txt") == 0) return strcat(cf.txt, "\r\n\r\n");
    else if(strcmp(suffix, "png") == 0) return strcat(cf.png, "\r\n\r\n");
    else if(strcmp(suffix, "gif") == 0) return strcat(cf.gif, "\r\n\r\n");
    else if(strcmp(suffix, "jpg") == 0) return strcat(cf.jpg, "\r\n\r\n");
    else if(strcmp(suffix, "jpeg") == 0) return strcat(cf.jpg, "\r\n\r\n");
    else if(strcmp(suffix, "css") == 0) return strcat(cf.css, "\r\n\r\n");
    else if(strcmp(suffix, "js") == 0) return strcat(cf.js, "\r\n\r\n");
    else if(strcmp(suffix, "ico") == 0) return strcat(cf.ico, "\r\n\r\n");
    else return cf.html;
}

// Reads in the config file and stores all the values in the cf struct
void populateConfig(){
    FILE *fp;
    char string1[1000], string2[1000];
    char *line = NULL;
    size_t len = 0;

    fp = fopen("ws.conf", "r");

    while(fscanf(fp, "%s", string1) != -1){
        // If the line is a comment
        if(string1[0] == '#'){
            getline(&line, &len, fp);
            continue;
        }
        // If not a comment, read in second value
        fscanf(fp, "%s", string2);

        if(strcmp(string1, "Listen") == 0){
            cf.port = atoi(string2);
        }
        else if(strcmp(string1, "DocumentRoot") == 0){
            strcpy(cf.docRoot, string2);
        }
        else if(strcmp(string1, "DirectoryIndex") == 0){
            strcpy(cf.dirIndex, string2);
        }
        else if(strcmp(string1, "Loc404") == 0){
            strcpy(cf.loc404, "errors/404notfound.html");
        }
        else if(strcmp(string1, "Loc501") == 0){
            strcpy(cf.loc501, "errors/501notimplemented.html");
        }
        else if(strcmp(string1, ".html") == 0){
            strcpy(cf.html, string2);
        }
        else if(strcmp(string1, ".htm") == 0){
            strcpy(cf.htm, string2);
        }
        else if(strcmp(string1, ".txt") == 0){
            strcpy(cf.txt, string2);
        }
        else if(strcmp(string1, ".png") == 0){
            strcpy(cf.png, string2);
        }
        else if(strcmp(string1, ".gif") == 0){
            strcpy(cf.gif, string2);
        }
        else if(strcmp(string1, ".jpg") == 0){
            strcpy(cf.jpg, string2);
        }
        else if(strcmp(string1, ".css") == 0){
            strcpy(cf.css, string2);
        }
        else if(strcmp(string1, ".js") == 0){
            strcpy(cf.js, string2);
        }
        else if(strcmp(string1, ".ico") == 0){
            strcpy(cf.ico, string2);
        }
    }
    free(line);
    fclose(fp);
}

// This is the function that reaps zombie processes and causes a clean exit and frees resources
// It handles all interrupts in general
void intHandler(int sig){
    // The signal is two when the user enters ctrl+c
    if(sig == 2){
        close(sock);
        running = false;
    }
    // When the child status has changed
    else if(sig == 17){
        int saved_errno = errno;
        while (waitpid((pid_t)(-1), 0, WNOHANG) > 0);
        errno = saved_errno;
    }
}

