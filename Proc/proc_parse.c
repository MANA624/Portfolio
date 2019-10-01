#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_DATA 9

void printInfo();
void aggregateData(int, int);
void getData(int*);
void burnData(FILE*, char, int);
float *averageArr(int, int[][NUM_DATA], int);

int main(int argc, char* argv[]){
    int read_rate;
    int printout_rate;

    if(argc == 1){
        printInfo();
    }else if(argc == 2){
        printf("Not enough arguments\n");
    }
    else{
        read_rate = atoi(argv[1]);
        printout_rate = atoi(argv[2]);
        if(read_rate == 0 || printout_rate == 0){
            printf("Invalid arguments\n");
            exit(1);
        }
        while(1){
            aggregateData(read_rate, printout_rate);
        }
    }

    return 0;
}

/*
    This function, when called, satisfies the default version of
    the program that runs when no arguments are present. It
    prints out the processor type, the OS kernel version, the
    total amount of memory associated with the machine, and the
    number of seconds that the machine has been running for. It
    does this all through the /proc filesystem
*/
void printInfo(){

    FILE *cpuinfo;
    char *arg = 0;
    size_t size = 0;
    char *signal;

    // Start off by getting the processor type
    cpuinfo = fopen("/proc/cpuinfo", "rb");
    signal = "model name	: ";
    while(getdelim(&arg, &size, '\n', cpuinfo) != -1){
        if(strncmp(signal, arg, strlen(signal)) == 0){
            printf("Processor Type: %s", arg+strlen(signal));
            break;
        }
    }

    // Next we get the kernel version
    cpuinfo = fopen("/proc/version", "rb");
    while(getdelim(&arg, &size, 0, cpuinfo) != -1){
        printf("%s", arg);
    }

    // Next we find out how much main memory is associate with the system
    cpuinfo = fopen("/proc/meminfo", "rb");
    signal = "MemTotal:";
    getdelim(&arg, &size, '\n', cpuinfo);
    printf("Amount of memory: %s", arg+strlen(signal));

    // Finally we find out how long the system has been running for
    cpuinfo = fopen("/proc/uptime", "rb");
    getdelim(&arg, &size, ' ', cpuinfo);
    printf("The system has been up for %sseconds\n", arg);

    // Free up memory
    free(arg);
    free(cpuinfo);
}

void aggregateData(int read_rate, int printout_rate){
    int num_iters = printout_rate / read_rate;
    int i, j;
    int arr[num_iters][NUM_DATA];
    float *results;

    for(i=0; i<num_iters; i++){
        for(j=0; j<NUM_DATA; j++){
            arr[i][j] = 0;
        }
    }

    // Collect the data for each data point
    for(i=0; i<num_iters; i++){
        sleep(read_rate);
        getData(arr[i]);
    }

    // Then we have to do math on the collected data
    results = averageArr(num_iters, arr, read_rate);

    // Now we just print out the results
    printf("Processor spent %.2f%% in user mode, %.2f%% in system mode, and %.2f%% idle\n", results[0], results[1], results[2]);
    printf("Total memory free: %.0f\t\tPercetage free: %.2f\n", results[3], results[4]);
    printf("The number of sectors/s of read: %.2f\twrite: %.2f\n", results[5], results[6]);
    printf("The rate of context switches/s: %.2f\n", results[7]);
    printf("Rate of process creations/s: %.2f\n\n", results[8]);
}

void getData(int *arr){
    FILE *readFile;
    char *arg = 0;
    size_t size = 0;
    char *signal;
    // Check out /proc/meminfo
    // Check out /proc/stat

    // Start by getting clock cycles in each mode
    readFile = fopen("/proc/stat", "rb");
    // Start with cycles in user mode
    burnData(readFile, ' ', 2);
    getdelim(&arg, &size, ' ', readFile);
    arr[0] = atoi(arg);
    // Now get cycles in kernel mode
    burnData(readFile, ' ', 1);
    getdelim(&arg, &size, ' ', readFile);
    arr[1] = atoi(arg);
    // Now get cycles idling
    getdelim(&arg, &size, ' ', readFile);
    arr[2] = atoi(arg);

    // Now we get the total amount of memory
    readFile = fopen("/proc/meminfo", "rb");
    signal = "MemTotal:";
    getdelim(&arg, &size, '\n', readFile);
    arr[4] = atoi(arg+strlen(signal));
    // Now get the amount of free memory
    signal = "MemFree:";
    getdelim(&arg, &size, '\n', readFile);
    arr[3] = atoi(arg+strlen(signal));

    // Get the number of sectors of r/w in the system
    readFile = fopen("/proc/diskstats", "rb");
    signal = "   1       0 ";
    arr[5] = arr[6] = 0;
    while(getdelim(&arg, &size, '\n', readFile) != -1){
        fseek(readFile, strlen(signal), SEEK_CUR);
        getdelim(&arg, &size, ' ', readFile);
        if(strncmp(arg, "sd", 2)){
            continue;
        }
        burnData(readFile, ' ', 2);
        getdelim(&arg, &size, ' ', readFile);
        arr[5] += atoi(arg);
        burnData(readFile, ' ', 3);
        getdelim(&arg, &size, ' ', readFile);
        arr[6] += atoi(arg);
    }

    // Get the number of processes and context switches
    readFile = fopen("/proc/stat", "rb");
    signal = "ctxt";
    while(getdelim(&arg, &size, '\n', readFile) != -1){
        if(strncmp(signal, arg, strlen(signal)) == 0){
            break;
        }
    }
    arr[7] = atoi(arg+strlen(signal));
    burnData(readFile, '\n', 1);
    signal = "processes";
    getdelim(&arg, &size, '\n', readFile);
    arr[8] = atoi(arg+strlen(signal));

    // Free up memory
    free(arg);
    free(readFile);
}

/*
    This is a helper function that I have written to help me to parse through data.
    It takes a string and a delimiter and parses num_burns times, discarding whatever
    the results were, then returning the original string
*/
void burnData(FILE *file, char delim, int num_burns){
    int i;
    char *arg = 0;
    size_t size = 0;

    for(i=0; i<num_burns; i++){
        getdelim(&arg, &size, delim, file);
    }
}

float *averageArr(int num_iters, int arr[num_iters][NUM_DATA], int read_rate){
    int i, j;
    static float ret[NUM_DATA] = {5.5, 4.4, 3.3, 2.2, 1.1, 6.6, 7.7, 8.8};
    unsigned long long int total;
    int temp[NUM_DATA-1];

    // We need to subtract the values from each other to get recent stats
    for(i=0; i<num_iters-1; i++){
        arr[i][0] = arr[i+1][0] - arr[i][0];
        arr[i][1] = arr[i+1][1] - arr[i][1];
        arr[i][2] = arr[i+1][2] - arr[i][2];
        arr[i][5] = arr[i+1][5] - arr[i][5];
        arr[i][6] = arr[i+1][6] - arr[i][6];
        arr[i][7] = arr[i+1][7] - arr[i][7];
        arr[i][8] = arr[i+1][8] - arr[i][8];
    }

    // Actually average the data together
    for(i=0; i<NUM_DATA; i++){
        total = 0;
        for(j=0; j<num_iters-1; j++){
            total += arr[j][i];
        }
        if(i==3 || i==4){
            total += arr[j][i];
            ret[i] = 1.*total /(num_iters);
        }
        else{
            ret[i] = 1.*total /(num_iters-1);
        }
    }
    // Calculate percentage of processor time
    total = ret[0] + ret[1] + ret[2];
    ret[0] /= total*.01;
    ret[1] /= total*.01;
    ret[2] /= total*.01;

    // Calculate percentage of memory used
    ret[4] = ret[3] / ret[4] * 100;

    // Normalize measurements to per second
    ret[5] = ret[5] / read_rate;
    ret[6] = ret[6] / read_rate;
    ret[7] = ret[7] / read_rate;
    ret[8] = ret[8] / read_rate;

    return ret;
}








