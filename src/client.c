#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>

#include "./include/parse.h"
#include "./include/log.h"

int nsecs;
time_t start_time;
char *fifoName;
bool closed_server = false;
pthread_mutex_t writeMutex;
pthread_mutex_t requestNumberMutex;

bool clientTimeout() {
    return (time(0) - start_time >= nsecs);
}

int generateNumber(int min, int max) {
    int number = rand() % (max - min + 1) + min;
    return number;
}

int fifoExists (char fifo_path[]) {
    struct stat buff;
    return (stat(fifo_path, &buff) == 0);
}

int writeToPublicFifo(Message *msg) {
    int fifo, ret_val = 0;

    pthread_mutex_lock(&writeMutex);

    if(!fifoExists(fifoName)) {

        fprintf(stderr, "fifo %s does not exist\n", fifoName);
        ret_val = 1;

    } else {
        while ((fifo = open(fifoName, O_WRONLY)) < 0); //sync
        write(fifo, msg, sizeof(Message));
        close(fifo);
    }

    pthread_mutex_unlock(&writeMutex);

    return ret_val;
}

int namePrivateFifo(Message *msg, char fifoPath[]){
    snprintf (fifoPath, 250, "/tmp/%u.%lu",msg->pid,msg->tid);
    return 0;
}

int creatPrivateFifo(char fifoPath[]){
    mkfifo(fifoPath, 0666);
    return 0;
}

int removePrivateFifo(char fifoPath[]){

    remove(fifoPath);

    return 0;
}

int readFromPrivateFifo(Message *msg, char fifoPath[]) {
    int fd;
    
    while ((fd = open(fifoPath, O_RDONLY | O_NONBLOCK)) < 0); //sync

    //as long as client is still open, will try to read
    while (!clientTimeout() && read(fd, msg, sizeof(Message)) <= 0);

    close(fd);

    if (clientTimeout()) return 1; //got out of the cycle due to client's timeout

    return 0; //got out of the cycle due to msg being successfully received
}

void *generateRequest(void * arg){
    char private_fifo[250];

    //to send to server
    Message msg;
    msg.pid = getpid();
    msg.tid = pthread_self();
    msg.rid = *( (int*) arg);
    msg.tskload = generateNumber(1, 9);
    msg.tskres = -1;

    pthread_mutex_unlock(&requestNumberMutex);

    //to receive from server
    //Message res;
    
    namePrivateFifo(&msg, private_fifo);
    creatPrivateFifo(private_fifo);

    //server's public fifo was closed, client could not send request
    if (writeToPublicFifo(&msg) != 0) { 

        closed_server = true;
        printf("Erro while sending request\n");

    } else logEvent(IWANT, msg); //request sent successfully to server

    //client could no longer wait for server's response

    if (!closed_server && readFromPrivateFifo(&msg, private_fifo) != 0) {
        
        logEvent(GAVUP, msg); 
        
    } else {

        //client's request was not attended due to server's timeout
        if (msg.tskres == -1 || closed_server){
            closed_server = true;
            logEvent(CLOSD, msg); 

        } else{
            //client's request successfully attended by the server
            logEvent(GOTRS, msg); 
        }

    }

    removePrivateFifo(private_fifo);
    
    return NULL;
}

void generateThreads() {
    int request_number = 0;
    pthread_t tid;
    pthread_t existing_threads[1000000];

    while(!clientTimeout() && !closed_server) {

        pthread_mutex_lock(&requestNumberMutex);
        pthread_create(&tid, NULL, generateRequest,(void*) &request_number);

        existing_threads[request_number] = tid;
        usleep(generateNumber(1000, 5000));
        ++request_number;

    }

    for(int i = 0 ; i < request_number; ++i){
        pthread_join(existing_threads[i], NULL);
    }
}

int main(int argc, char *argv[]) {
    start_time = time(0);

    //srand(time(NULL));
    srand(0);

    if (parseCommand(argc ,argv , &fifoName , &nsecs) != 0){
        fprintf(stderr, "client: parsing error\n");
        return 1;
    }
    
    if (pthread_mutex_init(&writeMutex, NULL) != 0){
        fprintf(stderr, "client: mutex init error\n");
        return 1;
    }

    if (pthread_mutex_init(&requestNumberMutex, NULL) != 0){
        fprintf(stderr, "client: mutex init error\n");
        return 1;
    }

    generateThreads();

    pthread_mutex_destroy(&writeMutex);
    pthread_mutex_destroy(&requestNumberMutex);

    return 0;
}
