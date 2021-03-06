#ifndef SRC_INCLUDE_LOG_H_
#define SRC_INCLUDE_LOG_H_

#include "./common.h"

typedef enum {
    IWANT, GOTRS, CLOSD, GAVUP
} event_t;

int logEvent(event_t event, const Message *msg);

#endif  // SRC_INCLUDE_LOG_H_
