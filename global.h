#ifndef GLOBAL_H
#define GLOBAL_H
#include <pthread.h>
#include "./protocol/findRoute.h"

extern pthread_mutex_t pathsMutex;
extern pthread_mutex_t peersMutex;
extern pthread_mutex_t channelsMutex;
extern pthread_mutex_t channelInfosMutex;
extern pthread_mutex_t* condMutex;
extern pthread_cond_t* condVar;
extern Array** paths;
extern unsigned short* condPaths;


#endif
