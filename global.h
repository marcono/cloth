#ifndef GLOBAL_H
#define GLOBAL_H
#include <pthread.h>
#include "./protocol/findRoute.h"
#include "./utils/list.h"

extern pthread_mutex_t pathsMutex;
extern pthread_mutex_t peersMutex;
extern pthread_mutex_t jobsMutex;
extern pthread_mutex_t condMutex[3000];
extern pthread_cond_t condVar[3000];
extern Array** paths;
extern unsigned short* condPaths;
extern Node* jobs;

#endif
