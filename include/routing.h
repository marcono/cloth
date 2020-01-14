#ifndef ROUTING_H
#define ROUTING_H

#include <stdint.h>

#include "array.h"
#include "heap.h"

struct distance{
  long node;
  uint64_t distance;
  uint64_t amt_to_receive;
  uint64_t fee;
};

struct dijkstra_hop {
  long node;
  long edge;
};

struct path_hop{
  long sender;
  long receiver;
  long edge;
};

struct route_hop {
  struct path_hop* path_hop;
  uint64_t amount_to_forward;
  uint16_t timelock;
};


struct route {
  uint64_t total_amount;
  uint32_t total_timelock;
  uint64_t total_fee;
  struct array *route_hops;
};


extern uint32_t** dist;
extern struct path_hop** next;

void initialize_dijkstra();


struct array* get_path(long source, long destination);


struct array* dijkstra_p(long source, long destination, uint64_t amount, struct array* ignored_nodes, struct array* ignored_edges, long p);

struct array* dijkstra(long source, long destination, uint64_t amount, struct array* ignored_nodes, struct array* ignored_edges);

struct route* transform_path_into_route(struct array* path_hops, uint64_t amount_to_send, int final_timelock);

void print_hop(struct route_hop* hop);

int compare_distance(struct distance* a, struct distance* b);

//struct array* find_paths(long source, long destination, double amount);

//struct array* find_paths(long source, long destination, double amount);


#endif
