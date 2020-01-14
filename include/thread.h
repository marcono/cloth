#ifndef THREAD_H
#define THREAD_H
#include <pthread.h>
#include "array.h"
#include "list.h"
#define PARALLEL 4

extern pthread_mutex_t paths_mutex;
extern pthread_mutex_t nodes_mutex;
extern pthread_mutex_t jobs_mutex;
extern struct array** paths;
extern struct element* jobs;

#endif
