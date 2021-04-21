#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "./include/parse.h"
#include "./include/log.h"

int generateNumber(int min, int max){

    int number = rand() % (max - min + 1) + min;

    return number;
}

void *generateRequest(){
    Message msg;
    msg.pid = getpid();
    msg.tid = pthread_self();
    msg.rid = generateNumber(1,9);
    msg.tskload = -1;
    msg.tskres = -1;

    logEvent(IWANT,msg);


    printf("TODO\n");
    return NULL;
}

int main(int argc, char *argv[]){
    srand(time(NULL));
    char *fifoname;
    int nsecs;
    time_t start_time = time(0);
    pthread_t tid = 1;

    if( parseCommand(argc ,argv , &fifoname , &nsecs) != 0){
        fprintf(stderr, "client: parsing error\n");
        return 1;
    }

    while(time(0) - start_time < nsecs){
        pthread_create(&tid, NULL, generateRequest, NULL);
        tid++;
        usleep(generateNumber(10,50));
    }

    return 0;
}
