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

extern long edge_index, peer_index, channel_index, payment_index;
extern double p_uncoop_before_lock, p_uncoop_after_lock;
extern gsl_rng *r;
extern const gsl_rng_type * T;
extern gsl_ran_discrete_t* uncoop_before_discrete, *uncoop_after_discrete;
FILE *csv_peer, *csv_edge, *csv_channel;
extern long n_dijkstra;

struct policy {
  uint32_t fee_base;
  uint32_t fee_proportional;
  uint32_t min_htlc;
  uint16_t timelock;
};

struct ignored{
  long id;
  uint64_t time;
};

struct peer {
  long id;
  int withholds_r;
  uint64_t initial_funds;
  uint64_t remaining_funds;
  struct array* edge;
  struct array* ignored_peers;
  struct array* ignored_edges;
};

struct channel {
  long id;
  long peer1;
  long peer2;
  long edge_direction1;
  long edge_direction2;
  uint64_t capacity;
  uint32_t latency;
  unsigned int is_closed;
};

struct edge {
  long id;
  long channel_id;
  long counterparty;
  long other_edge_direction_id;
  struct policy policy;
  uint64_t balance;
  unsigned int is_closed;
};

struct payment {
  long id;
  long sender;
  long receiver;
  uint64_t amount; //millisatoshis
  struct route* route;
  int is_success;
  int uncoop_after;
  int uncoop_before;
  int is_timeout;
  uint64_t start_time;
  uint64_t end_time;
  int attempts;
};

extern struct array* peers;
extern struct array* edges;
extern struct array* channels;

void initialize_protocol_data(long n_peers, long n_edges, double p_uncoop_before, double p_uncoop_after, double RWithholding, int gini, int sigma, long capacity_per_channel, unsigned int is_preproc);

struct peer* create_peer(long id, long edges_size);

struct peer* create_peer_post_proc(long id, int withholds_r);

struct channel* create_channel(long id, long peer1, long peer2, uint32_t latency);

struct channel* create_channel_post_proc(long id, long direction1, long direction2, long peer1, long peer2, uint64_t capacity, uint32_t latency);

struct edge* create_edge(long id, long channel_id, long counterparty, struct policy policy);

struct edge* create_edge_post_proc(long id, long channel_id, long other_direction, long counterparty, uint64_t balance, struct policy policy);

struct payment* create_payment(long id, long sender, long receiver, uint64_t amount);

void connect_peers(long peer1, long peer2);

void create_topology_from_csv(unsigned int is_preproc);

int is_present(long element, struct array* long_array);

int is_equal(long *a, long *b);

uint64_t compute_fee(uint64_t amount_to_forward, struct policy policy);

void find_route(struct event* event);

void send_payment(struct event* event);

void forward_payment(struct event* event);

void receive_payment(struct event* event);

void forward_success(struct event* event);

void receive_success(struct event* event);

void forward_fail(struct event* event);

void receive_fail(struct event* event);

void* dijkstra_thread(void*arg);

long get_edge_index(struct peer*n);

#endif
