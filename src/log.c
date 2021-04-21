#include "./include/log.h"
#include "stdio.h"
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

void logEvent(event_t event, Message msg) {
    switch (event) {
    case IWANT: logIwant(msg); break;
    case GOTRS: logGotrs(msg); break;
    case CLOSD: logClosd(msg); break;
    case GAVUP: logGavup(msg); break;
    default: break;
    }
}

// inst ; rid ; tskload ; pid ; tid ; tskres ; oper
void logIwant(Message msg) {
    time_t elapsed;
    time(&elapsed);

    printf("%ld ; %d ; %d ; %d ; %ld; %d ; IWANT",
    elapsed, msg.rid, msg.tskload, getpid(), msg.tid, msg.tskres);
}

void logGotrs(Message msg) {
    time_t elapsed;
    time(&elapsed);

    printf("%ld ; %d ; %d ; %d ; %ld ; %d ; GOTRS",
    elapsed, msg.rid, msg.tskload, getpid(), msg.tid, msg.tskres);
}

void logClosd(Message msg) {
    time_t elapsed;
    time(&elapsed);

    printf("%ld ; %d ; %d ; %d ; %ld ; %d ; CLOSD",
    elapsed, msg.rid, msg.tskload, getpid(), msg.tid, msg.tskres);
}

void logGavup(Message msg) {
    time_t elapsed;
    time(&elapsed);

    printf("%ld ; %d ; %d ; %d ; %ld ; %d ; GAVUP",
    elapsed, msg.rid, msg.tskload, getpid(), msg.tid, msg.tskres);
}
