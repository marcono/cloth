#ifndef ROUTING_H
#define ROUTING_H

#include <stdint.h>
#include "array.h"
#include "heap.h"
#include <pthread.h>
#include "array.h"
#include "list.h"
#include "network.h"

#define N_THREADS 4

extern pthread_mutex_t data_mutex;
extern pthread_mutex_t jobs_mutex;
extern struct array** paths;
extern struct element* jobs;

struct thread_args{
  struct network* network;
  struct array* payments;
  int data_index;
};

struct distance{
  long node;
  uint64_t distance;
  uint64_t amt_to_receive;
  uint64_t fee;
  double probability;
  uint32_t timelock;
  double weight;
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



void initialize_dijkstra(long n_nodes, long n_edges, struct array* payments);

void run_dijkstra_threads(struct network* network, struct array* payments);

struct array* dijkstra(long source, long destination, uint64_t amount, struct array* ignored_nodes, struct array* ignored_edges, struct network* network, long p);

struct route* transform_path_into_route(struct array* path_hops, uint64_t amount_to_send, int final_timelock, struct network* network);

void print_hop(struct route_hop* hop);

int compare_distance(struct distance* a, struct distance* b);


#endif
