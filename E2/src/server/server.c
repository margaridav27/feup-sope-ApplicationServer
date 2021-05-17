#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include <sys/types.h>
#include <assert.h>
#include <poll.h>

#include "./include/parse.h"
#include "./include/log.h"
#include "./lib/lib.h"

#define NUMBER_OF_THREADS 100000000
#define BUFSZ_SIZE 5120

size_t bufsz;
size_t nsecs;
time_t start_time;
char *fifo_name;
int public;
Message * storage;
int storage_usage;
pthread_mutex_t storage_mutex;

size_t remainingTime() { return nsecs - (time(0) - start_time); }

int fifoExists(const char *fifo_path) {
    if (fifo_path == NULL) return -1;
    return access(fifo_path, F_OK) == 0;
}

/*int writeToPrivateFifo(const Message *msg) {
    if (msg == NULL) return -1;
    pthread_mutex_lock(&write_mutex);

    struct pollfd fd;
    const int kmsec = (int) remainingTime() * 1000;
    fd.fd = public;
    fd.events = POLLOUT;
    int r = poll(&fd, 1, kmsec);
    if (r < 0) {
        pthread_mutex_unlock(&write_mutex);
        perror("client: error opening private fifo");
        return -1;
    }

    if (r == 0) {
        pthread_mutex_unlock(&write_mutex);
        return -1;
    }

    if ((fd.revents & POLLOUT) && write(public, msg, sizeof(*msg)) < 0) {
        pthread_mutex_unlock(&write_mutex);
        perror("client: error writing to public fifo");
        return -1;
    }

    pthread_mutex_unlock(&write_mutex);

    return 0;
}*/

int creatPublicFifo() {
    if (fifo_name == NULL) return -1;
    return mkfifo(fifo_name, 0666);
}

/*int removePrivateFifo(char fifo_path[]) {
    if (fifo_path == NULL) return -1;
    return remove(fifo_path);
}*/

int namePrivateFifo(const Message *msg, char *fifo_path) {
    if (msg == NULL || fifo_path == NULL) return -1;
    return snprintf(fifo_path, 250, "/tmp/%u.%lu", msg->pid, msg->tid) > 0;
}


int readFromPublicFifo(Message *msg) {
    const int kmsec = (int) remainingTime() * 1000;
    struct pollfd fd;
    fd.fd = public;
    fd.events = POLL_IN;

    int r = poll(&fd, 1, kmsec);
    if (r < 0) {
        perror("client: error opening private fifo");
        //close(public);
        return -1;
    }

    if (r == 0) {
        //close(public);
        return 1;
    }

    if ((fd.revents & POLLIN) && read(public, msg, sizeof(Message)) < 0) {
        perror("client: error reading from private fifo");
        //close(public);
        return -1;
    }

    return 0;
}

int writeMessageToStorage(){

    return 0;
}


void *handleRequest(void *p) {
    if (p == NULL) return NULL;
    Message *msg = p;

    //char private_fifo[PATH_MAX] = {0};

    //namePrivateFifo(msg, private_fifo);

    //msg->tid = pthread_self();

    msg->tskres = task(msg->tskload);

    //write to armazem

    writeMessageToStorage();

    /*
    if (writeToPublicFifo(msg) == 0) {
        // successfully sent the request
        logEvent(IWANT, msg);
        if (readFromPrivateFifo(msg, private_fifo) != 0) {
            // client could no longer wait for server's response
            logEvent(GAVUP, msg);
        } else {
            // client's request was not attended due to server's timeout
            if (msg->tskres == -1) logEvent(CLOSD, msg);
            // client's request successfully attended by the server
            else
            logEvent(GOTRS, msg);
        }
    }*/
    free(p);

    return NULL;
}

/*int assembleMessage(int request_number, Message *msg) {
    if (msg == NULL) return -1;
    *msg = (Message) {
            .rid = request_number,
            .pid = getpid(),
            .tskload = generateNumber(1, 9),
            .tskres = -1,
    };
    return 0;
}*/

void generateThreads() {
    int request_number = 0;
    pthread_t tid;
    pthread_t *existing_threads = malloc(NUMBER_OF_THREADS * sizeof(*existing_threads));
    if (existing_threads == NULL) return;
    memset(existing_threads, 0, sizeof(*existing_threads));

    while (request_number < NUMBER_OF_THREADS &&
            remainingTime() > 0 ) {

        Message *msg = malloc(sizeof(*msg));

        if (msg == NULL) continue;
        if( readFromPublicFifo(msg) == 0 ){
            pthread_create(&tid, NULL, handleRequest, msg);
            existing_threads[request_number] = tid;
            ++request_number;
        }
    }

    for (int i = 0; i < request_number; ++i)
        pthread_join(existing_threads[i], NULL);
    free(existing_threads);
}

int creatStorage(){
    storage_usage = 0;
    storage = malloc(bufsz * sizeof(Message));
    if (storage == NULL) return -1;
    memset(storage, 0, sizeof(*storage));
    return 0;
}

int main(int argc, char *argv[]) {
    start_time = time(NULL);

    printf("Initial time: %ld", start_time);
    fflush(stdout);

    if (parseCommand(argc, argv, &fifo_name, &nsecs, &bufsz)) {
        fprintf(stderr, "client: parsing error\n");
        return 1;
    }

    
    if (pthread_mutex_init(&storage_mutex, NULL)) {
        perror("client: mutex init");
        return 1;
    }

    if(creatPublicFifo() != 0){
        perror("server: error creating public fifo");
        return 1;
    }

    while (remainingTime() > 0 &&
        (public = open(fifo_name, O_RDONLY | O_NONBLOCK)) < 0) continue;

    if (public < 0) {
        fprintf(stderr, "client: opening private fifo: took too long\n");
        return -1;
    }

    if(bufsz == 0){
       bufsz = BUFSZ_SIZE;
    }

    if(creatStorage() != 0){
        perror("server: error creating storage");
        return -1;
    }

    generateThreads();

    if (close(public) < 0) {
        perror("client: error closing private fifo");
        return -1;
    }

    if (pthread_mutex_destroy(&storage_mutex)) {
        perror("client: mutex destroy");
        return 1;
    }

    return 0;
}
