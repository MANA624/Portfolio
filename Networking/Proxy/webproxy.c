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
#include <openssl/md5.h>
#include <dirent.h>
#include <time.h>
/* You will have to modify the program below */

#define MAXBUFSIZE 524288
#define SIZECACHE 600

void saveIP(const char *domain, const char *IP);
char *checkHost(const char *domain);
void linkPrefetch(const char *contents, const char *url, int port);
const char *getContentType(char suffix[]);
void populateConfig();
char *get_filename_ext(const char *filename);
void intHandler(int sig);
int checkURLExists(const char url[]);
bool allowedDomain(const char *domain);
int inCache(const char *domain);
void storeCache(const char *domain, const char *contents, int length);
char *str2md5(const char *str, int length);

char blockedPages[100][35];
bool running = true;
int sock;
int timeout;
char retrieved[MAXBUFSIZE];

int main (int argc, char * argv[] )
{
	struct sockaddr_in sin, client;      //"Internet socket address structure"
	struct sockaddr_in from_site;
	struct hostent *hostInfo = NULL;
	struct sigaction sa;                        // Struct to handle zombie processes
	char buf[MAXBUFSIZE], contents[MAXBUFSIZE]; //a buffer to store our received message
	socklen_t struct_length = sizeof(client);   //length of the sockaddr_in structure
	char method[500];
	char URL[500];
	char protocol[500];
	char *domain;
	char requestPath[500];
	char domainHard[500];
	char URLHard[500], URL2[500];
	char *temp = NULL;
	char *serverIP;
	int on = 1;
    int connfd;
    int servSock;
    bool portProvided;
    int port;
    int i, j;
    int length;

	if(argc < 2){
        printf("USAGE: ./webproxy <server_port> &\n");
        exit(1);
    }
    if(argc > 2){
        timeout = atoi(argv[2]);
    }
    else{
        timeout = 99999;
    }

    // Ensure that the directory exists. If it does, rm -rf it. Then create it
    if (opendir("cache"))
    {
        /* Directory exists. */
        system("rm -rf cache");
    }
    mkdir("cache", S_IRWXG | S_IRWXO | S_IRWXU);

    populateConfig("forbidden.txt");

    // Pass in functions to struct handler to handle all interrupts
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sa.sa_handler = intHandler;
    sigaction(SIGCHLD, &sa, 0);
    sigaction(SIGINT, &sa, NULL);

	// Info about the address struct
	bzero(&sin,sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(atoi(argv[1]));
	sin.sin_addr.s_addr = INADDR_ANY;

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
        connfd = accept(sock, (struct sockaddr*)&client, &struct_length);

        if(!fork()){
            // This child will die, so throw its socks away
            close(sock);

            read(connfd, buf, MAXBUFSIZE-1);

            //printf("Buffer: %s\n", buf);

            // Parse the method, requested URL, and (hopefully) HTTP version
            sscanf(buf, "%s %s %s", method, URL, protocol);
            strcpy(URLHard, URL);

            if(method == NULL);
            else if((strcmp(protocol, "HTTP/1.0") && strcmp(protocol, "HTTP/1.1")) || strncmp(URL, "http://", 7)){
                printf("Not a supported protocol\n");
                write(connfd, "400 : BAD REQUEST\n", 18);
            }
            else if(strcmp(method, "GET") == 0){
                strcpy(URL2, URL);

                portProvided = false;

                for(i=7; i<strlen(URL); i++){
                    if(URL[i] == ':'){
                        portProvided = true;
                        break;
                    }
                }

                temp = strtok(URL, "//");
                if(!portProvided){
                    port = 80;
                    domain = strtok(NULL,"/");
                }
                else{
                    domain = strtok(NULL,":");
                    temp = strtok(NULL, "/");
                    port = atoi(temp);
                }

                strcpy(domainHard, domain);

                temp = strtok(URL2, "//");
                temp = strtok(NULL, "/");
                if(temp != NULL){
                    temp = strtok(NULL, "\0");
                }
                strcpy(requestPath, "/");
                if(temp != NULL){
                    strcat(requestPath, temp);
                }

                // Check if the domain is valid
                printf("Domain: %s\n", domainHard);

                serverIP = NULL;
                if((serverIP = checkHost(domain)) == NULL){
                    hostInfo = gethostbyname(domain);
                }

                // Check if we can send
                if(serverIP == NULL && hostInfo == NULL){
                    printf("Not a valid host\n");
                    write(connfd, "NOT A VALID HOST\n", 17);
                }
                else if(!allowedDomain(domainHard)){
                    printf("Domain %s not allowed!\n", domainHard);
                    write(connfd, "ERROR 403: FORBIDDEN\n", 21);
                }
                else if((length = inCache(URLHard)) != -1){
                    printf("Item is in cache!\n");
                    i = 500;
                    while(i<length){
                        write(connfd, &retrieved[i-500], 500);
                        i += 500;
                    }
                    i -= 500;
                    write(connfd, &retrieved[i], length-i);

                }
                else{
                    if(serverIP == NULL){
                        serverIP = inet_ntoa(*(struct in_addr*)*hostInfo->h_addr_list);
                        saveIP(domain, serverIP);
                    }
                    bzero((char*)&from_site, sizeof(from_site));
                    from_site.sin_family = AF_INET;
                    from_site.sin_port = htons(port);
                    from_site.sin_addr.s_addr = inet_addr(serverIP);
                    //bcopy((char*)hostInfo->h_addr,(char*)&from_site.sin_addr.s_addr,hostInfo->h_length);


                    // Causes the system to create a generic socket of type TCP
                    servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                    setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
                    connect(servSock, (struct sockaddr *) &from_site, struct_length);

                    if(servSock < 0){
                        printf("I quit\n");
                        exit(1);
                    }

                    //bzero(buf, MAXBUFSIZE);

                    sprintf(buf, "GET %s %s\r\nHost: %s\r\nConnection: close\r\n\r\n", requestPath, protocol, domainHard);

                    printf("Sending: %s\n", buf);
                    if(write(servSock, buf, strlen(buf)) < 0){
                        printf("Error in sending to server! .%s.\n", domainHard);
                        write(connfd, "ERROR IN SENDING REQUEST\n", 25);
                    }
                    else{
                        bzero(contents, MAXBUFSIZE);
                        length = 0;
                        do{
                            bzero(buf, MAXBUFSIZE);
                            i = read(servSock, buf, 500);
                            if(i > 0){
                                for(j=0; j<i; j++){
                                    contents[length+j] = buf[j];
                                }
                                length += i;
                                write(connfd, buf, i);
                            }
                        }while(i > 0);
                        storeCache(URLHard, contents, length);
                        //linkPrefetch(contents, URLHard, atoi(argv[1]));
                    }
                }
            }
            else{
                printf("Unsupported method: %s\n", method);
            }

            close(servSock);
            close(connfd);
            printf("\n\n");
            exit(0);
        }
        close(connfd);
    }

	exit(0);
}

void saveIP(const char *domain, const char *IP){
    FILE *fp;
    fp = fopen("cache/hosts.txt", "a");

    fprintf(fp, "%s\t%s\n", domain, IP);

    fclose(fp);
}

char *checkHost(const char *domain){
    FILE *fp;
    char string1[1000], string2[1000];
    char *line = NULL;

    fp = fopen("cache/hosts.txt", "r");
    if(fp == NULL) return NULL;

    while(fscanf(fp, "%s\t%s", string1, string2) != -1){
        if(strcmp(string1, domain) == 0){
            return strdup(string2);
        }
    }
    free(line);
    fclose(fp);
    return NULL;
}

void linkPrefetch(const char *contents, const char *url, int port){
    int res, end;
    char *str = strdup(contents);
    char *domain = NULL;
    struct sockaddr_in server;
    int sock;
    int on = 1;
    char buf[200];
    char *currentDir = strdup(url);
    end = strrchr(currentDir, '/')-currentDir;
    currentDir[end+1] = '\0';
    char *big = strdup(url);
    end = strchr(big+7, '/')-big;
    big[end] = '\0';
    char requestPath[200];
    int i;


    printf("The URL: %s\n", url);
    printf("The cuurent dir: %s\n", currentDir);
    printf("domain: %s\n", big);

    while((res = strstr(str, "href=\"")-str) > 0){
        end = strstr(str+res+6, "\"")-str;
        domain = strdup(str+res+6);
        domain[end-res-6] = '\0';
        str += end+2;
        printf("Res: %d\t%d\n", res, end);
        printf("Domain: %s\n", domain);

        if(strncmp("http://", domain, 7) == 0){
            strcpy(requestPath, domain);
        }
        else if(strncmp("/", domain, 1) == 0){
            strcpy(requestPath, big);
            strcat(requestPath, domain);
        }
        else{
            strcpy(requestPath, currentDir);
            strcat(requestPath, domain);
        }

        sprintf(buf, "GET %s HTTP/1.1\r\n\r\n", requestPath);

        bzero(&server, sizeof(server));
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        server.sin_addr.s_addr = inet_addr("127.0.0.1");

        // Causes the system to create a generic socket of type TCP
        if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("unable to create socket");
            continue;
        }
        if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) < 0){
            printf("setsockopt() failed!\n");
            continue;
        }

        if(connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0){
            printf("Connection failed!\n");
            continue;
        }

        i = write(sock, buf, strlen(buf));
        printf("I: %d\n", i);
    }




}

int inCache(const char *domain){
    int i;
    char *hash = str2md5(domain, strlen(domain));
    char filename[50];
    FILE *fp;
    int currentChar;
    time_t timestamp;
    char cTime[12];
    char l[20];
    int length;

    strcpy(filename, "cache/");
    strcat(filename, hash);
    printf("Filename: %s\n", filename);
    fp = fopen(filename, "rb");
    if(fp == NULL){
        return -1;
    }
    else{
        for(i=0; i<10; i++){
            cTime[i] = getc(fp);
        }
        timestamp = atoi(cTime);
        // If cache is invalidated
        if(time(NULL) > timestamp){
            fclose(fp);
            remove(filename);
            return -1;
        }
        else{
            i = 0;
            while(true){
                currentChar = getc(fp);
                if(currentChar < '0' || currentChar > '9'){
                    break;
                }
                l[i] = currentChar;
                i++;
            }
            length = atoi(l);
            retrieved[0] = currentChar;
            //i = 1;
            //while((currentChar = getc(fp)) != EOF){
            /*printf("Length: %s\t%d\n", l, length);
            for(i=1; i<=length; i++){
                currentChar = getc(fp);
                retrieved[i] = currentChar;
            }
            */
            fread(&retrieved[1], length, 1, fp);
        }
    }
    fclose(fp);
    return length;
}

void storeCache(const char *domain, const char *contents, int length){
    char *hash = str2md5(domain, strlen(domain));
    char filename[50];
    FILE *fp;
    char timer[12];
    char l[20];
    sprintf(timer, "%ld", time(NULL)+timeout);
    sprintf(l, "%d", length);
    int i;

    strcpy(filename, "cache/");
    strcat(filename, hash);
    fp = fopen(filename, "wb");

    fwrite(timer, 1, strlen(timer), fp);
    fwrite(l, 1, strlen(l), fp);

    //fwrite(contents, 1, strlen(contents)-1, fp);
    if(fp){
        for(i=0; i<length; i++){
            fwrite(&contents[i], 1, 1, fp);
        }
    }

    fclose(fp);
}

// Reads in the config file and stores all the values in the cf struct. Ez
void populateConfig(char confFile[]){
    FILE *fp;
    char string1[1000];
    char *line = NULL;
    size_t len = 0;
    int i = 0;

    fp = fopen(confFile, "r");

    while(fscanf(fp, "%s", string1) != -1){
        // If the line is a comment
        if(string1[0] == '#'){
            getline(&line, &len, fp);
            continue;
        }
        else{
            strcpy(blockedPages[i], string1);
            i++;
        }
    }
    free(line);
    fclose(fp);
}

// Check to see if the requested web page is not allowed
bool allowedDomain(const char *domain){
    int i;
    int numRows = sizeof(blockedPages[0]) / sizeof(blockedPages[0][0]);
    for(i=0; i<numRows; i++){
        if(strcmp(blockedPages[i], domain) == 0){
            return false;
        }
    }
    return true;
}

// A helper function to check and see if the URL requested actually is valid
int checkURLExists(const char url[]){
    char test[150];
    strcpy(test, "wget --spider -q ");
    strcat(test, url);
    int res = system(test);
    return !res;
}

// Retrieve the extension of a filename
char *get_filename_ext(const char *filename) {
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
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

// A hash function that I got from Stackoverflow. Very similar to the one
// that I used in past assignments, only it takes a string, not a filename,
// which is what I need it to do
char *str2md5(const char *str, int length){
    int n;
    MD5_CTX c;
    unsigned char digest[16];
    char *out = (char*)malloc(33);

    MD5_Init(&c);

    while (length > 0) {
        if (length > 512) {
            MD5_Update(&c, str, 512);
        } else {
            MD5_Update(&c, str, length);
        }
        length -= 512;
        str += 512;
    }

    MD5_Final(digest, &c);

    for (n = 0; n < 16; ++n) {
        snprintf(&(out[n*2]), 1024, "%02x", (unsigned int)digest[n]);
    }

    return out;
}

