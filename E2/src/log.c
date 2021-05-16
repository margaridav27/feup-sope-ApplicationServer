#include "./include/log.h"

#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

int logEvent(event_t event, const Message *msg) {
    static const char *kevent_names[] = {"RECVD", "TSKEX", "TSKDN", "2LATE", "FAILD"};
    time_t elapsed = time(NULL);
    printf("%ld ; %d ; %d ; %d ; %ld; %d ; %s\n",
           elapsed,
           msg->rid,
           msg->tskload,
           msg->pid,
           msg->tid,
           msg->tskres,
           kevent_names[event]);
    return fflush(stdout);
}
