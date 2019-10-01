#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define BUFFER_LENGTH 256
#define TABLE_SIZE 326

int main(int argc, char *argv[]){
    // Initialize variables
    int fd;
    int ret;
    char *filename = "/dev/identify";
    int input;
    int i;
    char receive[TABLE_SIZE] = "";
    // Open the file. Check to make sure it opened
    fd = open(filename, O_RDONLY);
    if(fd == -1){
        printf("Error opening device driver!\n");
        exit(1);
    }

    ret = read(fd, receive, TABLE_SIZE);
    if(ret < 0){
        printf("Error reading from module\n");
    }
    else{
        printf("The following was different: %s\n", receive);
    }

    close(fd);

}