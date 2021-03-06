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

#define NUMBER_OF_THREADS 100000000
size_t nsecs;
time_t start_time;
char *fifoName;
int public;
pthread_mutex_t write_mutex;

size_t remainingTime() { return nsecs - (time(0) - start_time); }

int generateNumber(int min, int max) {
    assert(min < max);
    return rand() % (max - min + 1) + min;
}

int fifoExists(const char *fifo_path) {
    if (fifo_path == NULL) return -1;
    return access(fifo_path, F_OK) == 0;
}

int writeToPublicFifo(const Message *msg) {
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
}

int namePrivateFifo(const Message *msg, char *fifo_path) {
    if (msg == NULL || fifo_path == NULL) return -1;
    return snprintf(fifo_path, 250, "/tmp/%u.%lu", msg->pid, msg->tid) > 0;
}

int creatPrivateFifo(const char *fifo_path) {
    if (fifo_path == NULL) return -1;
    return mkfifo(fifo_path, 0666);
}

int removePrivateFifo(char fifo_path[]) {
    if (fifo_path == NULL) return -1;
    return remove(fifo_path);
}

int readFromPrivateFifo(Message *msg, const char *fifo_path) {
    int f = 0;
    while (remainingTime() > 0 &&
         (f = open(fifo_path, O_RDONLY | O_NONBLOCK)) < 0) continue;
    if (f < 0) return 1;

    const int kmsec = (int) remainingTime() * 1000;
    struct pollfd fd;
    fd.fd = f;
    fd.events = POLL_IN;

    int r = poll(&fd, 1, kmsec);
    if (r < 0) {
        perror("client: error opening private fifo");
        close(f);
        return -1;
    }

    if (r == 0) {
        close(f);
        return 1;
    }

    if ((fd.revents & POLLIN) && read(f, msg, sizeof(Message)) < 0) {
        perror("client: error reading from private fifo");
        close(f);
        return -1;
    }

    if (close(f) < 0) {
        perror("client: error closing private fifo");
        return -1;
    }

    return 0;
}

void *generateRequest(void *p) {
    if (p == NULL) return NULL;
    Message *msg = p;
    msg->tid = pthread_self();

    char private_fifo[PATH_MAX] = {0};

    namePrivateFifo(msg, private_fifo);
    creatPrivateFifo(private_fifo);

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
    }
    free(p);

    removePrivateFifo(private_fifo);
    return NULL;
}

int assembleMessage(int request_number, Message *msg) {
    if (msg == NULL) return -1;
    *msg = (Message) {
            .rid = request_number,
            .pid = getpid(),
            .tskload = generateNumber(1, 9),
            .tskres = -1,
    };
    return 0;
}

void generateThreads() {
    int request_number = 0;
    pthread_t tid;
    pthread_t *existing_threads = malloc(NUMBER_OF_THREADS * sizeof(*existing_threads));
    if (existing_threads == NULL) return;
    memset(existing_threads, 0, sizeof(*existing_threads));

    while (request_number < NUMBER_OF_THREADS &&
            remainingTime() > 0 && fifoExists(fifoName)) {
        Message *msg = malloc(sizeof(*msg));
        if (msg == NULL || assembleMessage(request_number, msg)) continue;
        pthread_create(&tid, NULL, generateRequest, msg);
        existing_threads[request_number] = tid;
        ++request_number;
        usleep(generateNumber(1000, 5000));
    }

    for (int i = 0; i < request_number; ++i)
        pthread_join(existing_threads[i], NULL);
    free(existing_threads);
}

int main(int argc, char *argv[]) {
    start_time = time(NULL);
    srand(start_time);

    printf("Initial time: %ld", start_time);
    fflush(stdout);

    if (parseCommand(argc, argv, &fifoName, &nsecs)) {
        fprintf(stderr, "client: parsing error\n");
        return 1;
    }

    if (pthread_mutex_init(&write_mutex, NULL)) {
        perror("client: mutex init");
        return 1;
    }

    while (remainingTime() > 0 &&
        (public = open(fifoName, O_WRONLY | O_NONBLOCK)) < 0) continue;

    if (public < 0) {
        fprintf(stderr, "client: opening private fifo: took too long\n");
        return -1;
    }

    generateThreads();

    if (close(public) < 0) {
        perror("client: error closing private fifo");
        return -1;
    }

    if (pthread_mutex_destroy(&write_mutex)) {
        perror("client: mutex destroy");
        return 1;
    }

    return 0;
}
