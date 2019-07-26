#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../utils/array.h"
#include "routing.h"
//#include "../utils/hash_table.h"
#include "../simulator/event.h"
#include "gsl_rng.h"
#include "gsl_randist.h"
#include <stdint.h>
#include <pthread.h>

extern long edge_index, peer_index, edge_info_index, payment_index;
extern double p_uncoop_before_lock, p_uncoop_after_lock;
extern gsl_rng *r;
extern const gsl_rng_type * T;
extern gsl_ran_discrete_t* uncoop_before_discrete, *uncoop_after_discrete;
FILE *csv_peer, *csv_edge, *csv_edge_info;
extern long n_dijkstra;

typedef struct policy {
  uint32_t fee_base;
  uint32_t fee_proportional;
  uint32_t min_hTLC;
  uint16_t timelock;
} Policy;

typedef struct ignored{
  long ID;
  uint64_t time;
} Ignored;

typedef struct peer {
  long ID;
  int withholds_r;
  uint64_t initial_funds;
  uint64_t remaining_funds;
  Array* edge;
  Array* ignored_peers;
  Array* ignored_edges;
} Peer;

typedef struct edge_info {
  long ID;
  long peer1;
  long peer2;
  long edge_direction1;
  long edge_direction2;
  uint64_t capacity;
  uint32_t latency;
  unsigned int is_closed;
} Channel;

typedef struct edge {
  long ID;
  long edge_info_id;
  long counterparty;
  long other_edge_direction_id;
  Policy policy;
  uint64_t balance;
  unsigned int is_closed;
} Edge;

typedef struct payment{
  long ID;
  long sender;
  long receiver;
  uint64_t amount; //millisatoshis
  Route* route;
  int is_success;
  int uncoop_after;
  int uncoop_before;
  int is_timeout;
  uint64_t start_time;
  uint64_t end_time;
  int attempts;
} Payment;

extern Array* peers;
extern Array* edges;
extern Array* edge_infos;

void initialize_protocol_data(long n_peers, long n_edges, double p_uncoop_before, double p_uncoop_after, double RWithholding, int gini, int sigma, long capacity_per_channel, unsigned int is_preproc);

Peer* create_peer(long ID, long edges_size);

Peer* create_peer_post_proc(long ID, int withholds_r);

Channel* create_edge_info(long ID, long peer1, long peer2, uint32_t latency);

Channel* create_edge_info_post_proc(long ID, long direction1, long direction2, long peer1, long peer2, uint64_t capacity, uint32_t latency);

Edge* create_edge(long ID, long edge_info_id, long counterparty, Policy policy);

Edge* create_edge_post_proc(long ID, long edge_info_id, long other_direction, long counterparty, uint64_t balance, Policy policy);

Payment* create_payment(long ID, long sender, long receiver, uint64_t amount);

void connect_peers(long peer1, long peer2);

void create_topology_from_csv(unsigned int is_preproc);

int is_present(long element, Array* long_array);

int is_equal(long *a, long *b);

uint64_t compute_fee(uint64_t amount_to_forward, Policy policy);

void find_route(Event* event);

void send_payment(Event* event);

void forward_payment(Event* event);

void receive_payment(Event* event);

void forward_success(Event* event);

void receive_success(Event* event);

void forward_fail(Event* event);

void receive_fail(Event* event);

void* dijkstra_thread(void*arg);

long get_edge_index(Peer*n);

#endif
