#include "./include/log.h"
#include "stdio.h"
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

int logEvent(event_t event, const Message *msg) {
    static const char *kevent_names[] = {"IWANT", "GOTRS", "CLOSD", "GAVUP"};
    time_t elapsed = time(NULL);

    return printf("%ld ; %d ; %d ; %d ; %ld; %d ; %s\n",
                  elapsed,
                  msg->rid,
                  msg->tskload,
                  msg->pid,
                  msg->tid,
                  msg->tskres,
                  kevent_names[event]
    );

}
