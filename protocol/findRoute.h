#ifndef FINDROUTE_H
#define FINDROUTE_H
#include "../utils/array.h"
#include <stdint.h>
#include "../utils/heap.h"


typedef struct distance{
  long peer;
  uint64_t distance;
  uint64_t amtToReceive;
  uint64_t fee;
} Distance;

typedef struct dijkstraHop {
  long peer;
  long channel;
} DijkstraHop;

typedef struct pathHop{
  long sender;
  long receiver;
  long channel;
} PathHop;

typedef struct routeHop {
  PathHop* pathHop;
  uint64_t amountToForward;
  uint16_t timelock;
} RouteHop;


typedef struct route {
  uint64_t totalAmount;
  uint32_t totalTimelock;
  uint64_t totalFee;
  Array *routeHops;
} Route;

/* Distance* distance; */
/* DijkstraHop* previousPeer; */
/* Heap* distanceHeap; */

extern uint32_t** dist;
extern PathHop** next;

void initializeDijkstra();

void floydWarshall();

Array* getPath(long source, long destination);

Array* findPaths(long source, long destination, double amount);

Array* dijkstraP(long source, long destination, uint64_t amount, Array* ignoredPeers, Array* ignoredChannels, long p);

Array* dijkstra(long source, long destination, uint64_t amount, Array* ignoredPeers, Array* ignoredChannels);

Route* transformPathIntoRoute(Array* pathHops, uint64_t amountToSend, int finalTimelock);

void printHop(RouteHop* hop);

int compareDistance(Distance* a, Distance* b);
 

#endif
