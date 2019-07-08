#ifndef GLOBAL_H
#define GLOBAL_H
#include <pthread.h>
#include "./protocol/routing.h"
#include "./utils/list.h"
#define PARALLEL 4

extern pthread_mutex_t paths_mutex;
extern pthread_mutex_t peers_mutex;
extern pthread_mutex_t jobs_mutex;
extern Array** paths;
extern Node* jobs;

#endif
