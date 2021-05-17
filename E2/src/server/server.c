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
#include <semaphore.h>
#include <assert.h>
#include <poll.h>
#include <mqueue.h>

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

sem_t empty; // how many slots are free
sem_t full; // how many slots have content
pthread_mutex_t read_mutex; // make buffer addition/removal atomic

mqd_t queue;

size_t remainingTime() { return nsecs - (time(0) - start_time); }

int fifoExists(const char *fifo_path) {
    if (fifo_path == NULL) return -1;
    return access(fifo_path, F_OK) == 0;
}

int queueExists() {
    return access("./communication_queue", F_OK) == 0;
}


int namePrivateFifo(const Message *msg, char *fifo_path) {
    if (msg == NULL || fifo_path == NULL) return -1;
    return snprintf(fifo_path, 250, "/tmp/%u.%lu", msg->pid, msg->tid) > 0;
}

int writeToPrivateFifo(const Message *msg) {
    int private;
    char private_fifo[PATH_MAX] = {0};

    namePrivateFifo(msg, private_fifo);

    if (fifoExists(private_fifo) != 0){
        perror("server: fifo does not exist");
        return -1;
    }

    while (remainingTime() > 0 &&
        (private = open(private_fifo, O_WRONLY | O_NONBLOCK)) < 0) continue;

    if (msg == NULL) return -1;
    //pthread_mutex_lock(&write_mutex);

    struct pollfd fd;
    const int kmsec = (int) remainingTime() * 1000;
    fd.fd = private;
    fd.events = POLLOUT;
    int r = poll(&fd, 1, kmsec);
    if (r < 0) {
        //pthread_mutex_unlock(&write_mutex);
        perror("client: error opening private fifo");
        return -1;
    }

    if (r == 0) {
        //pthread_mutex_unlock(&write_mutex);
        return -1;
    }

    if ((fd.revents & POLLOUT) && write(private, msg, sizeof(*msg)) < 0) {
        //pthread_mutex_unlock(&write_mutex);
        perror("client: error writing to public fifo");
        return -1;
    }

    logEvent(TSKDN,msg);

    //pthread_mutex_unlock(&write_mutex);

    return 0;
}

int creatPublicFifo() {
    if (fifo_name == NULL) return -1;
    return mkfifo(fifo_name, 0666);
}

/*int removePrivateFifo(char fifo_path[]) {
    if (fifo_path == NULL) return -1;
    return remove(fifo_path);
}*/


int readFromPublicFifo(Message *msg) {
    pthread_mutex_lock(&read_mutex);

    const int kmsec = (int) remainingTime() * 1000;
    struct pollfd fd;
    fd.fd = public;
    fd.events = POLL_IN;

    int r = poll(&fd, 1, kmsec);
    if (r < 0) {
        pthread_mutex_unlock(&read_mutex);
        perror("client: error opening private fifo");
        //close(public);
        return -1;
    }

    if (r == 0) {
        pthread_mutex_unlock(&read_mutex);
        //close(public);
        return 1;
    }

    if ((fd.revents & POLLIN) && read(public, msg, sizeof(Message)) < 0) {
        pthread_mutex_unlock(&read_mutex);
        perror("client: error reading from private fifo");
        //close(public);
        return -1;
    }

    pthread_mutex_unlock(&read_mutex);
    return 0;
}

int writeMessageToStorage(Message *msg){ 

    if(queueExists() != 0){
        fprintf(stderr, "Queue no longer exists\n");
    }

    sem_wait(&empty); // decrease the number of empty slots

    mq_send(queue, (char *) msg, sizeof(*msg), 1); //send message to queue

    sem_post(&full); // increase the number of slots with content

    return 0;
}

int readMessageFromStorage(Message *msg){

    if(queueExists() != 0){
        fprintf(stderr, "Queue no longer exists\n");
        return 1;
    }

    sem_wait(&full);// decrease the number of full slots

    if(mq_receive(queue,  (char *)  msg, sizeof(*msg), NULL) == -1){
        perror("server: error while writing to queue");
        return 1;
    }

    sem_post(&empty); // increase the number of slots without content

    return 0;
}


void *handleRequest(void *p) {
    if (p == NULL) return NULL;
    Message *msg = p;

    //msg->tid = pthread_self();

    msg->tskres = task(msg->tskload);

    logEvent(TSKEX,msg);

    writeMessageToStorage(msg);

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
            logEvent(RECVD,msg);
            pthread_create(&tid, NULL, handleRequest, msg);
            existing_threads[request_number] = tid;
            ++request_number;
        }

        if(readMessageFromStorage(msg)) continue;
        //read from storage
        //receive msg
        writeToPrivateFifo(msg);
       
    }

    for (int i = 0; i < request_number; ++i)
        pthread_join(existing_threads[i], NULL);
    free(existing_threads);
}

int createStorage(){
    struct mq_attr attr;
    attr.mq_curmsgs = 0;
    attr.mq_maxmsg = bufsz;
    attr.mq_msgsize = sizeof(Message);
    attr.mq_flags = 0;

    queue = mq_open("./communication_queue", O_RDWR | O_CREAT, 0666, &attr);

    return queue;//ERROR CHECK
}


int main(int argc, char *argv[]) {
    start_time = time(NULL);


    printf("Initial time: %ld", start_time);
    fflush(stdout);
    
    if (parseCommand(argc, argv, &fifo_name, &nsecs, &bufsz)) {
        fprintf(stderr, "client: parsing error\n");
        return 1;
    }


    if (pthread_mutex_init(&read_mutex, NULL)) {
        perror("client: mutex init");
        return 1;
    }

    if(bufsz == 0){
       bufsz = BUFSZ_SIZE;
    }

    if(createStorage() == -1){
        perror("server: error creating queue");
        return 1;
    }

    sem_init(&empty, 0, bufsz);

	sem_init(&full, 0, 0);

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

    
    if(createStorage() != 0){
        perror("server: error creating storage");
        return -1;
    }

    generateThreads();

    if (close(public) < 0) {
        perror("client: error closing private fifo");
        return -1;
    }

    sem_destroy(&empty);

    sem_destroy(&full);

    if (pthread_mutex_destroy(&read_mutex)) {
        perror("client: mutex destroy");
        return 1;
    }

    mq_close(queue);
    mq_unlink("./communication_queue");

    return 0;
}
