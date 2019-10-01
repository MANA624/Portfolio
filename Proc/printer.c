#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]){
    //volatile char toPrint[] = "This is my initial string ";
    volatile char *ptr = malloc(27 * sizeof(char));
    ptr = "This is my initial string ";
    printf("My PID is: %d\nAddress is %p\n", getpid(), ptr);

    while(1){
        printf("%s\n", ptr);
        sleep(3);
    }

    return 0;
}
