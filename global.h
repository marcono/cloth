#ifndef GLOBAL_H
#define GLOBAL_H
#include <pthread.h>
#include "./protocol/routing.h"
#include "./utils/list.h"
#define PARALLEL 4

extern pthread_mutex_t paths_mutex;
extern pthread_mutex_t nodes_mutex;
extern pthread_mutex_t jobs_mutex;
extern struct array** paths;
extern struct element* jobs;

#endif
