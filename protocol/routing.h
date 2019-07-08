#ifndef FINDROUTE_H
#define FINDROUTE_H
#include "../utils/array.h"
#include <stdint.h>
#include "../utils/heap.h"


typedef struct distance{
  long peer;
  uint64_t distance;
  uint64_t amt_to_receive;
  uint64_t fee;
} Distance;

typedef struct dijkstra_hop {
  long peer;
  long channel;
} Dijkstra_hop;

typedef struct path_hop{
  long sender;
  long receiver;
  long channel;
} Path_hop;

typedef struct route_hop {
  Path_hop* path_hop;
  uint64_t amount_to_forward;
  uint16_t timelock;
} Route_hop;


typedef struct route {
  uint64_t total_amount;
  uint32_t total_timelock;
  uint64_t total_fee;
  Array *route_hops;
} Route;


extern uint32_t** dist;
extern Path_hop** next;

void initialize_dijkstra();


Array* get_path(long source, long destination);


Array* dijkstra_p(long source, long destination, uint64_t amount, Array* ignored_peers, Array* ignored_channels, long p);

Array* dijkstra(long source, long destination, uint64_t amount, Array* ignored_peers, Array* ignored_channels);

Route* transform_path_into_route(Array* path_hops, uint64_t amount_to_send, int final_timelock);

void print_hop(Route_hop* hop);

int compare_distance(Distance* a, Distance* b);

//Array* find_paths(long source, long destination, double amount);

//Array* find_paths(long source, long destination, double amount);


#endif
