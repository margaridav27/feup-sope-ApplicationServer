#include "./include/parse.h"

#include <stdio.h>  // fprintf()
#include <stdlib.h>  // strtol()
#include <stdbool.h>  // bool
#include <string.h>  // memset(), strlen()
#include <sys/types.h>  // mode_t
#include <unistd.h>  // getopt(), optind

int parseCommand(int argc, char *argv[], char* fifoname[], size_t *nsecs, size_t *bufsz) {
    if (argv == NULL || fifoname == NULL || nsecs == NULL || bufsz == NULL) return -1;

    // incorrect number of arguments
    if (argc != 4 && argc != 6) {
        fprintf(stderr, "client: invalid number of arguments\n");
        return -1;
    }

    char* timeString = "-t";
    if (strcmp(argv[1], timeString)) {
        fprintf(stderr, "client: invalid time string\n");
        return -1;
    }

    *nsecs = atoi(argv[2]);
    if (*nsecs <= 0) {
        fprintf(stderr, "client: invalid time value\n");
        return 1;
    }

    if(argc == 6){
        char* timeString = "-l";
        if (strcmp(argv[3], timeString)) {
            fprintf(stderr, "client: invalid size string\n");
            return -1;
        }

        *bufsz = atoi(argv[4]);
        if (*bufsz <= 0) {
            fprintf(stderr, "client: invalid size value\n");
            return 1;
        }

        *fifoname = argv[5];
    }
    else{
        *fifoname = argv[3];
        *bufsz = 0;
    }

    return 0;
}
