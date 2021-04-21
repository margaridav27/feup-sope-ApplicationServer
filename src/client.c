#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#include "./include/parse.h"

int generateNumber(int min, int max){

    int number = rand()%(max-min) + min;
    return number;
}

int main(int argc, char *argv[]){
    srand(time(NULL));
    char *fifoname;
    int nsecs;
    time_t start_time = time(0);

    if( parseCommand(argc ,argv , &fifoname , &nsecs) != 0){
        fprintf(stderr, "client: parsing error\n");
        return 1;
    }

    while(time(0) - start_time < nsecs){
        sleep(1);
        printf("hello");
        fflush(stdout);
    }

    return 0;
}
