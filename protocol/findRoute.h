#ifndef FINDROUTE_H
#define FINDROUTE_H
#include "../utils/array.h"

typedef struct distance{
  long peer;
  double distance;
} Distance;


typedef struct hop{
  long peer;
  long channel;
} Hop;

Array* findPaths(long source, long destination, double amount);

Array* dijkstra(long source, long destination, double amount, Array* ignoredPeers, Array* ignoredChannels);

#endif
