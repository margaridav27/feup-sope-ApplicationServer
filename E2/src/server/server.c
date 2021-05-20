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
#include <sys/queue.h>

#include "./include/parse.h"
#include "./include/log.h"
#include "./lib/lib.h"

#define NUMBER_OF_THREADS 100000000
#define BUF_SIZE 5120


typedef struct node {
    Message *msg;
    STAILQ_ENTRY(node) nodes;
} node_t;

STAILQ_HEAD(stailthead_t, node); //template for head
struct stailthead_t head; //define queue head

size_t bufsz;
size_t nsecs;
time_t start_time;
int public;

sem_t empty; // how many slots are free
sem_t full; // how many slots have content
pthread_mutex_t read_mutex; // make buffer addition/removal atomic


size_t remainingTime() { return nsecs - (time(0) - start_time); }

int fifoExists(const char *fifo_path) {
    if (fifo_path == NULL) return -1;
    return access(fifo_path, F_OK) == 0;
}

int namePrivateFifo(const Message *msg, char *fifo_path) {
    if (msg == NULL || fifo_path == NULL) return -1;
    return snprintf(fifo_path, 250, "/tmp/%u.%lu", msg->pid, msg->tid) > 0;
}

int removePublicFifo(const char *fifo_name) {
    return remove(fifo_name);
}


int writeToPrivateFifo(Message *msg) {
    if (msg == NULL) return -1;

    char private_fifo_name[PATH_MAX] = {0};
    namePrivateFifo(msg, private_fifo_name);

    /*  if (fifoExists(private_fifo_name) != 0) {
          perror("server: private fifo does not exist");
          msg->tskres = -1;
          logEvent(FAILD, msg);
          return -1;
      }*/

    int private = 0;
    while (remainingTime() > 0 && (private = open(private_fifo_name, O_WRONLY | O_NONBLOCK)) < 0) continue;

    struct pollfd fd;
    const int kmsec = (int) remainingTime() * 1000;
    fd.fd = private;
    fd.events = POLLOUT;

    int r = poll(&fd, 1, kmsec);

    if (r < 0 || (fd.revents & POLLHUP)) {
        perror("server: error opening private fifo");
        msg->tskres = -1;
        logEvent(FAILD, msg);
        close(private);
        return -1;
    }

    if (r == 0) {
        close(private);
        return -1;
    }

    if ((fd.revents & POLLOUT) && write(private, msg, sizeof(*msg)) < 0) {
        perror("server: error writing to private fifo");
        close(private);
        msg->tskres = -1;
        logEvent(FAILD, msg);
        return -1;
    }

    logEvent(TSKDN, msg);
    close(private);
    return 0;
}

int creatPublicFifo(const char *fifo_name) {
    if (fifo_name == NULL) return -1;
    return mkfifo(fifo_name, 0666);
}

int readFromPublicFifo(Message *msg) {
    if (msg == NULL) return -1;

    pthread_mutex_lock(&read_mutex);

    const int kmsec = (int) remainingTime() * 1000;
    struct pollfd fd;
    fd.fd = public;
    fd.events = POLLIN;

    int r = poll(&fd, 1, kmsec);

    if (r < 0) {
        pthread_mutex_unlock(&read_mutex);
        perror("server: error opening public fifo");
        return -1;
    }

    if (r == 0) {
        pthread_mutex_unlock(&read_mutex);
        return -1;
    }

    if ((fd.revents & POLLIN) && read(public, msg, sizeof(Message)) < 0) {
        pthread_mutex_unlock(&read_mutex);
        perror("server: error reading from public fifo");
        return -1;
    }

    pthread_mutex_unlock(&read_mutex);
    return (fd.revents & POLLHUP);
}


int createStorage() {
    STAILQ_INIT(&head);
    return 0;
}


int writeMessageToStorage(Message *msg) {
    if (msg == NULL) return -1;

    node_t *n = malloc(sizeof(*n));
    if (n == NULL) return -1;
    sem_wait(&empty); // decrease the number of empty slots

    n->msg = msg;

    //insert at end of queue
    STAILQ_INSERT_TAIL(&head, n, nodes);

    sem_post(&full); // increase the number of slots with content

    return 0;
}

int readMessageFromStorage(Message *msg) {
    node_t *n;
    sem_wait(&full);// decrease the number of full slots

    //if queue is empty return
    if (STAILQ_EMPTY(&head)) {
        return 1;
    }
    n = STAILQ_FIRST(&head);
    STAILQ_REMOVE_HEAD(&head, nodes);
    memcpy(msg, n->msg, sizeof(*msg));

    sem_post(&empty); // increase the number of slots without content

    free(n);
    return 0;
}

void *consumeTask(void *p) {
    Message *msg = malloc(sizeof(*msg));

    while (remainingTime() > 0) {
        if (readMessageFromStorage(msg) > 0) {
            perror("reading from storage");
            continue;
        }
        writeToPrivateFifo(msg);
    }

    free(msg);
    return NULL;
}

void *handleRequest(void *p) {
    if (p == NULL) return NULL;
    Message *msg = p;
    msg->tskres = task(msg->tskload);
    logEvent(TSKEX, msg);
    writeMessageToStorage(msg);
    return NULL;
}

void generateThreads() {
    int request_number = 0;
    pthread_t consumer;
    pthread_create(&consumer, NULL, consumeTask, NULL);
    pthread_t tid;
    pthread_t *existing_threads = malloc(NUMBER_OF_THREADS * sizeof(*existing_threads));
    if (existing_threads == NULL) return;
    memset(existing_threads, 0, sizeof(*existing_threads));

    while (request_number < NUMBER_OF_THREADS && remainingTime() > 0) {
        Message *msg = malloc(sizeof(*msg));
        if (msg == NULL) continue;
        if (readFromPublicFifo(msg) == 0) {
            logEvent(RECVD, msg);
            pthread_create(&tid, NULL, handleRequest, msg);
            existing_threads[request_number] = tid;
            ++request_number;
        }

        //readMessageFromStorage();
        //read from storage
        //receive msg
        writeToPrivateFifo(msg);
    }

    /* // handling pendent requests after server's timeout
     while (readFromPublicFifo(msg) == 0) {
         msg->tskres = -1;
         if (writeToPrivateFifo(msg) != 0) return;
         logEvent(_2LATE, msg);
     }*/

    for (int i = 0; i < request_number; ++i) pthread_join(existing_threads[i], NULL);
    pthread_join(consumer, NULL);
    free(existing_threads);
}

int main(int argc, char *argv[]) {

    signal(SIGPIPE, SIG_IGN);

    start_time = time(NULL);
    printf("Initial time: %ld", start_time);
    fflush(stdout);

    char *fifo_name;
    if (parseCommand(argc, argv, &fifo_name, &nsecs, &bufsz)) {
        fprintf(stderr, "server: parsing error\n");
        return 1;
    }

    if (pthread_mutex_init(&read_mutex, NULL)) {
        perror("server: mutex init");
        return 1;
    }

    if (bufsz == 0) bufsz = BUF_SIZE;

    if (creatPublicFifo(fifo_name) != 0) {
        perror("server: error creating public fifo");
        return 1;
    }

    sem_init(&empty, 0, bufsz);
    sem_init(&full, 0, 0);

    while (remainingTime() > 0 && (public = open(fifo_name, O_RDONLY | O_NONBLOCK)) < 0) continue;
    if (public < 0) {
        fprintf(stderr, "server: opening public fifo: took too long\n");
        return -1;
    }
    createStorage();
    generateThreads();

    if (close(public) < 0) {
        perror("server: error closing public fifo");
        return -1;
    }

    if (sem_destroy(&empty) | sem_destroy(&full)) {
        perror("server: sem destroy");
        return 1;
    }

    if (pthread_mutex_destroy(&read_mutex)) {
        perror("server: mutex destroy");
        return 1;
    }
    if (removePublicFifo(fifo_name)) return 1;

    return 0;
}
