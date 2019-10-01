#ifndef TESTER_H
#define TEESTER_H
#include <linux/ioctl.h>


struct procinfo
{
    pid_t pid;
    pid_t ppid;
    struct timespec start_time;
    int num_sib;
} ;

#endif