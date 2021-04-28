#ifndef PROJECT_INCLUDE_PARSE_H_
#define PROJECT_INCLUDE_PARSE_H_

#include <sys/types.h>

int parseCommand(int argc, char *argv[], char *fifoname[], size_t *nsecs);

#endif
