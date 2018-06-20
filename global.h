#ifndef GLOBAL_H
#define GLOBAL_H
#include <pthread.h>
#include "./protocol/findRoute.h"
#include "./utils/list.h"
#define PARALLEL 8

extern pthread_mutex_t pathsMutex;
extern pthread_mutex_t peersMutex;
extern pthread_mutex_t jobsMutex;
extern Array** paths;
extern Node* jobs;

#endif
