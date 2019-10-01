#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

void funcMal(int**);
int counting(int, bool);

char* printer = "XXGLOBALZZ: After freeing memory";

int main(int argc, char *argv[]){
    printf("The process ID is: %d\n", getpid());
    printf("This program is useful to inspect memory\nPress any key to continue\n");
    int pointer_len = 150;
    int *arr[pointer_len];
    char *dummy;
    int i;
    int fact;

    printf("Before first malloc");
    getchar();
    arr[0] = malloc(1024);
    printf("%d\n", sizeof(int));
    printf("Address of first alloc: %p\nPreceeding bytes:\n", arr[0]);
    for(i=0; i<16; i++){
        printf(" %02x", *( ((char *)arr[0]-16) + i));
    }
    getchar();

    for (i = 0; i<148; i++)
    {
        arr[i+1] = malloc(1024);
    }
    printf("After second malloc, before recursive function call");
    getchar();

    fact = counting(100000, true);

    printf("%dAfter calling recursive function\nBefore malloc function call", fact);
    getchar();
    funcMal(&arr[149]);
    printf("After malloc called inside function return");
    getchar();

    // Free up all the space at the end
    for(i=0; i<pointer_len; i++){
        free(arr[i]);
    }

    printf("%s\n", printer);
    getchar();

    return 0;
}

void funcMal(int **allocate){
    printf("Before malloc inside function");
    getchar();
    *allocate = malloc(1000*sizeof(int));
    printf("After malloc inside function");
    getchar();
}

int counting(int n, bool toPrint){
    // Print the first time the function runs
    if(toPrint){
        printf("At the start of function");
        getchar();
    }

    if(n <= 1){
        printf("Inside deepest function call");
        getchar();
        return 1;
    }
    else{
        return 1 + counting(n - 1, false);
    }
}
