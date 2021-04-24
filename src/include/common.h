#ifndef _COMMON_H
#define _COMMON_H 1

#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

typedef struct {
	int rid; 		// request id
	int tskload;	// task load
	pid_t pid; 		// process id
	pthread_t tid;	// thread id
	int tskres;		// task result
} Message;

#endif // _COMMON_H
