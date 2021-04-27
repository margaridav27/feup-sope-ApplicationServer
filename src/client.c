#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <poll.h>

#include "./include/parse.h"
#include "./include/log.h"

int nsecs;
time_t start_time;
char *fifoName;
pthread_mutex_t write_mutex;
pthread_mutex_t request_number_mutex;

int remainingTime() { return nsecs - (time(0) - start_time); }

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
    if (!fifoExists(fifoName)) return -1;

    // This call shall block until the FIFO is opened for writing by the server.
    int fd = open(fifoName, O_WRONLY);
    pthread_mutex_unlock(&write_mutex);
    if (fd < 0) {
        perror("client: error opening public fifo");
        return -1;
    }

    if (write(fd, msg, sizeof(*msg)) < 0) {
        perror("client: error writing to public fifo");
        return -1;
    }

    // COMBACK: This instruction causes the server to not terminate.
    //close(fd);

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
    return unlink(fifo_path);
}

int readFromPrivateFifo(Message *msg, const char *fifo_path) {
    struct pollfd fd;
    const int kmsec = remainingTime() * 1000;

    fd.fd = open(fifo_path, O_RDONLY);
    fd.events = POLL_IN | POLLHUP;
    int r = poll(&fd, 1, kmsec);
    if (r < 0) {
        perror("client: error opening private fifo");
        return -1;
    }

    if ((fd.revents & POLLIN) && read(fd.fd, msg, sizeof(Message)) < 0) {
        perror("client: error reading from private fifo");
        return -1;
    }

    //COMBACK: Somehow, this causes the server to break down.
    if (close(fd.fd) < 0) {
        perror("client: error closing private fifo");
        return -1;
    }

    return 0; //got out of the cycle due to msg being successfully received
}

void *generateRequest(void *arg) {
    char private_fifo[250];

    //to send to server
    Message msg = (Message) {
            .rid = *(int *) arg,
            .pid = getpid(),
            .tid = pthread_self(),
            .tskload = generateNumber(1, 9),
            .tskres = -1,
    };

    pthread_mutex_unlock(&request_number_mutex);

    namePrivateFifo(&msg, private_fifo);
    creatPrivateFifo(private_fifo);

    if (writeToPublicFifo(&msg) == 0) {
        logEvent(IWANT, &msg); //successfully sent the request
        if (readFromPrivateFifo(&msg, private_fifo) != 0) {
            logEvent(GAVUP, &msg); //client could no longer wait for server's response
        } else {
            if (msg.tskres == -1) {
                logEvent(CLOSD, &msg); //client's request was not attended due to server's timeout
            } else logEvent(GOTRS, &msg); //client's request successfully attended by the server
        }
    }

    removePrivateFifo(private_fifo);
    return NULL;
}

void generateThreads() {
    int request_number = -1;
    pthread_t tid;
    pthread_t existing_threads[1000000];

    while (remainingTime() > 0 && fifoExists(fifoName)) {
        pthread_mutex_lock(&request_number_mutex);
        ++request_number;
        pthread_create(&tid, NULL, generateRequest, &request_number);
        existing_threads[request_number] = tid;
        usleep(generateNumber(1000, 5000));
    }

    for (int i = 0; i <= request_number; ++i) {
        pthread_join(existing_threads[i], NULL);
    }
}

int main(int argc, char *argv[]) {
    start_time = time(NULL);

    //srand(start_time);
    srand(0);

    if (parseCommand(argc, argv, &fifoName, &nsecs)) {
        fprintf(stderr, "client: parsing error\n");
        return 1;
    }

    if (pthread_mutex_init(&write_mutex, NULL) || pthread_mutex_init(&request_number_mutex, NULL)) {
        perror("client: mutex init");
        return 1;
    }

    generateThreads();

    // Note: the bitwise or is used to ensure both commands run
    if (pthread_mutex_destroy(&write_mutex) | pthread_mutex_destroy(&request_number_mutex)) {
        perror("client: mutex destroy");
        return 1;
    }

    return 0;
}
