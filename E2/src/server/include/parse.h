#ifndef SRC_INCLUDE_PARSE_H_
#define SRC_INCLUDE_PARSE_H_

#include <sys/types.h>

int parseCommand(int argc, char *argv[], char *fifoname[], size_t *nsecs, size_t *bufsz);

#endif  // SRC_INCLUDE_PARSE_H_
