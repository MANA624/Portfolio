#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "tester.h"

int main(int argc, char *argv[]){
    // Initialize variables
    struct procinfo info;
    printf("The PID is %d\n", getpid());
    printf("My parent's PID is %d\n", getppid());
    int fd;
    char *filename = "/dev/getprocinfo";
    int input;

    // Read in integer from user
    printf("Enter an integer: ");
    scanf("%d", &input);

    // Open the file. Check to make sure it opened
    fd = open(filename, O_RDWR);
    if(fd == -1){
        printf("Error opening device driver!\n");
        exit(1);
    }

    printf("It actually kinda opened the device\n");

    ioctl(fd, input, &info);
    printf("The PID: %d\t\tParent Pid: %d\n", info.pid, info.ppid);
    printf("Process start time was at %ld seconds and %ld nanoseconds\n",
            (long)info.start_time.tv_sec, info.start_time.tv_nsec);
    printf("This process had %d siblings\n", info.num_sib);

    close(fd);

}