#ifndef FINDROUTE_H
#define FINDROUTE_H
#include "../utils/array.h"

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
  double amountToForward;
  int timelock;
} RouteHop;


typedef struct route {
  double totalAmount;
  int totalTimelock;
  double totalFee;
  Array *routeHops;
} Route;

Array* findPaths(long source, long destination, double amount);

Array* dijkstra(long source, long destination, double amount, Array* ignoredPeers, Array* ignoredChannels);

Route* transformPathIntoRoute(Array* pathHops, double amountToSend, int finalTimelock);

#endif
