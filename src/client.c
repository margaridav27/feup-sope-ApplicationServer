#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "./include/parse.h"
#include "./include/log.h"

char *fifoName;
bool closedService = false;

int generateNumber(int min, int max){

    int number = rand() % (max - min + 1) + min;

    return number;
}

int writeToFifo(Message *msg){
    int fifo;

    fifo = open(fifoName, O_WRONLY);
    write(fifo, &msg, sizeof(Message));
    close(fifo);

    logEvent(IWANT,*msg);

    return 0;
}

int creatPrivateFifo(Message *msg){
    char fifoPath[250];

    //buil private fifo path
    snprintf (fifoPath, 250, "/tmp/%u.%lu",msg->pid,msg->tid);

    //create private fifo
    mkfifo(fifoPath, 0666);

    return 0;
}

int removePrivateFifo(Message *msg){
    char fifoPath[250];

    //buil private fifo path
    snprintf (fifoPath, 250, "/tmp/%u.%lu",msg->pid,msg->tid);

    remove(fifoPath);

    return 0;
}

void *generateRequest(void * arg){

    Message msg;
    msg.pid = getpid();
    msg.tid = pthread_self();
    msg.rid = *( (int*) arg);
    msg.tskload = generateNumber(1,9);
    msg.tskres = -1;

    creatPrivateFifo(&msg);

    writeToFifo(&msg);

    logEvent(GOTRS,msg);

    removePrivateFifo(&msg);

    printf("TODO\n");

    //message receive from service;


    //verify if service stills open
    /*if(msg.tskres == -1){
        closedService = true;
    }*/

    return NULL;
}


int main(int argc, char *argv[]){

    int nsecs, requestNumber = 0;
    time_t start_time = time(0);
    pthread_t tid;

    srand(time(NULL));

    if( parseCommand(argc ,argv , &fifoName , &nsecs) != 0){
        fprintf(stderr, "client: parsing error\n");
        return 1;
    }
    
    //creat threads while time has not pass and service is open
    while(time(0) - start_time < nsecs && !closedService){

        pthread_create(&tid, NULL, generateRequest,(void*) &requestNumber);
        usleep(generateNumber(10,50));
        requestNumber += 1;

    }



    return 0;
}
