#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>

#define BUFFER_LENGTH 8192
#define TABLE_SIZE 326
#define LOG_FILE "/home/user/Dropbox/School/Advanced_OS/current/four/sys_table.txt"

void check_sys_calls();

int main(int argc, char *argv[]){
    // Initialize variables
    int fd;
    int ret;
    char *filename = "/dev/better";
    int input;
    int i;
    char receive[TABLE_SIZE] = "";
    // Open the file. Check to make sure it opened
    /*
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
     */

    check_sys_calls();
}

void check_sys_calls(){
    int first_time;
    FILE *fp;
    int fd;
    int ret;
    char receive[BUFFER_LENGTH];
    char *driverName = "/dev/better";

    system("sudo insmod better.ko");

    // Open the file. Check to make sure it opened
    fd = open(driverName, O_RDONLY);
    if(fd == -1){
        printf("Error opening device driver!\n");
        exit(1);
    }

    if(access(LOG_FILE, F_OK) == -1){
        // File doesn't exist
        printf("File doesn't exist\n");
        first_time = 1;
        strcpy(receive, "y");
    }
    else{
        // File exists
        printf("File exists\n");
        first_time = 0;
        fp = fopen(LOG_FILE, "r");
        fgets(receive, BUFFER_LENGTH, fp);
    }

    ret = read(fd, receive, TABLE_SIZE);
    if(ret < 0){
        printf("Error reading from module\n");
    }
    else{
        if(first_time){
            fp = fopen(LOG_FILE, "w");
            fputs(receive, fp);

            printf("The system calls were successfully logged\n");
        }
        else {
            printf("The following was different: %s\n", receive);
        }
    }
    close(fd);
    fclose(fp);

//    sleep(5);

    system("sudo rmmod better");
}
// 00007f31e4483264