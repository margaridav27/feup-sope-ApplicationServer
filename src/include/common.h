#ifndef SRC_INCLUDE_COMMON_H_
#define SRC_INCLUDE_COMMON_H_ 1

#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

typedef struct {
	int rid; 										// request id
	pid_t pid; 										// process id
	pthread_t tid;									// thread id
	int tskload;									// task load
	int tskres;										// task result
} Message;

#endif // SRC_INCLUDE_COMMON_H_
