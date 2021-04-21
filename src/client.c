#include <stdio.h>

#include "./include/parse.h"

int main(int argc, char *argv[]){
    char *fifoname;
    int nsecs;

    if( parseCommand(argc ,argv , &fifoname , &nsecs) != 0){
        fprintf(stderr, "client: parsing error\n");
        return 1;
    }

    return 0;
}
