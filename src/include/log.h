#ifndef PROJECT_INCLUDE_LOG_H_
#define PROJECT_INCLUDE_LOG_H_

#include "common.h"

typedef enum {
    IWANT, GOTRS, CLOSD, GAVUP
} event_t;

void logEvent(event_t event, Message msg);

void logIwant(Message msg);

void logGotrs(Message msg);

void logClosd(Message msg);

void logGavup(Message msg);

#endif