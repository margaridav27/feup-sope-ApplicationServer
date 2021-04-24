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

char *fifoName;
bool closedService = false;
bool closedClient = false;
pthread_mutex_t writeMutex;

int generateNumber(int min, int max)
{

    int number = rand() % (max - min + 1) + min;

    return number;
}

int writeToPublicFifo(Message *msg)
{
    int fifo;

    while ((fifo = open(fifoName, O_WRONLY)) < 0)
        ;

    pthread_mutex_lock(&writeMutex);

    write(fifo, &msg, sizeof(msg));

    pthread_mutex_unlock(&writeMutex);

    close(fifo);

    logEvent(IWANT, *msg);

    return 0;
}

int creatPrivateFifo(char fifoPath[])
{

    //create private fifo
    mkfifo(fifoPath, 0666);

    return 0;
}

int removePrivateFifo(char fifoPath[])
{

    remove(fifoPath);

    return 0;
}

int readFromPrivateFifo(Message *msg, char fifoPath[])
{
    int fd = open(fifoPath, O_RDONLY);
    int nbytes = -1;

    while (nbytes < 0)
    {

        nbytes = read(fd, &msg, sizeof(msg));
        printf("reading");
    }
    //TODO: add mutex or lock operation
    logEvent(GOTRS, *msg);

    close(fd);

    return 0;
}

int namePrivateFifo(Message *msg, char fifoPath[])
{
    //buil private fifo path
    snprintf(fifoPath, 250, "/tmp/%u.%lu", msg->pid, msg->tid);
    return 0;
}

void *generateRequest(void *arg) {
    Message msg;
    Message res;
    char private_fifo[250];
    msg.pid = getpid();
    msg.tid = pthread_self();
    msg.rid = *((int *)arg);
    msg.tskload = generateNumber(1, 9);
    msg.tskres = -1;

    namePrivateFifo(&msg, private_fifo);

    creatPrivateFifo(private_fifo);

    writeToPublicFifo(&msg);
    printf("fifo name %s\n", private_fifo);

    readFromPrivateFifo(&res, private_fifo);

    // TODO
    //verify if service is still open
    /*if(res.tskres == -1){
        closedService = true;
    }*/

    removePrivateFifo(private_fifo);

    return NULL;
}

int generateThreads(int nsecs, time_t start_time) {
    int request_number = 0;
    pthread_t tid;

    pthread_t existing_threads[1000000000];

    //creat threads while time has not passed and service is open
    while (time(0) - start_time < nsecs && !closedService) {
        pthread_create(&tid, NULL, generateRequest, (void *)&request_number);
        existing_threads[request_number] = tid;
        usleep(generateNumber(100, 200));
        request_number += 1;
    }

    closedClient = true;

    //check if thread is finished
    /*for(int i = 0 ; i < request_number; i++){
        if (pthread_kill(existing_threads[i], 0) == ESRCH){
            pthread_cancel(existing_threads[i]);
        }
    }*/

    for (int i = 0; i < request_number; i++) {
        pthread_join(existing_threads[i], NULL);
    }

    return 0;
}

int main(int argc, char *argv[]) {

    int nsecs;
    time_t start_time = time(0);

    srand(time(NULL));

    if (parseCommand(argc, argv, &fifoName, &nsecs) != 0) {
        fprintf(stderr, "client: parsing error\n");
        return 1;
    }

    if (pthread_mutex_init(&writeMutex, NULL) != 0) {
        fprintf(stderr, "client: mutex init error\n");
        return 1;
    }

    generateThreads(nsecs, start_time);

    pthread_mutex_destroy(&writeMutex);

    return 0;
}
