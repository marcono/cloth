#ifndef FINDROUTE_H
#define FINDROUTE_H
#include "../utils/array.h"
#include <stdint.h>

typedef struct distance{
  long peer;
  double distance;
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
  int timelock;
} RouteHop;


typedef struct route {
  uint64_t totalAmount;
  int totalTimelock;
  uint64_t totalFee;
  Array *routeHops;
} Route;

Array* findPaths(long source, long destination, double amount);

Array* dijkstra(long source, long destination, uint64_t amount, Array* ignoredPeers, Array* ignoredChannels);

Route* transformPathIntoRoute(Array* pathHops, uint64_t amountToSend, int finalTimelock);

void printHop(RouteHop* hop);

#endif
