#ifndef ROUTING_H
#define ROUTING_H

#include <stdint.h>
#include "array.h"
#include "heap.h"
#include <pthread.h>
#include "array.h"
#include "list.h"

#define N_THREADS 4

extern pthread_mutex_t paths_mutex;
extern pthread_mutex_t nodes_mutex;
extern pthread_mutex_t jobs_mutex;
extern struct array** paths;
extern struct element* jobs;

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



void initialize_dijkstra();

void run_dijkstra_threads();

struct array* dijkstra(long source, long destination, uint64_t amount, struct array* ignored_nodes, struct array* ignored_edges, long p);

struct route* transform_path_into_route(struct array* path_hops, uint64_t amount_to_send, int final_timelock);

void print_hop(struct route_hop* hop);

int compare_distance(struct distance* a, struct distance* b);


#endif
