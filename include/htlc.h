#ifndef HTLC_H
#define HTLC_H

#include <stdint.h>
#include <pthread.h>
#include "gsl_rng.h"
#include "gsl_randist.h"
#include "array.h"
#include "routing.h"
#include "event.h"
#include "cloth.h"

extern long edge_index, node_index, channel_index, payment_index;
extern double p_uncoop_before_lock, p_uncoop_after_lock;
extern gsl_ran_discrete_t* uncoop_before_discrete, *uncoop_after_discrete;
FILE *csv_node, *csv_edge, *csv_channel;
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

struct node {
  long id;
  struct array* open_edges;
  struct array* ignored_nodes;
  struct array* ignored_edges;
};

struct channel {
  long id;
  long node1;
  long node2;
  long edge1;
  long edge2;
  uint64_t capacity;
  uint32_t latency;
  unsigned int is_closed;
};

struct edge {
  long id;
  long channel_id;
  long counterparty;
  long other_edge_id;
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
  unsigned int is_success;
  unsigned int uncoop_after;
  unsigned int uncoop_before;
  unsigned int is_timeout;
  uint64_t start_time;
  uint64_t end_time;
  int attempts;
};

extern struct array* nodes;
extern struct array* edges;
extern struct array* channels;

void initialize_network(struct network_params net_params, unsigned int is_preproc);

struct node* create_node(long id, long edges_size);

struct node* create_node_post_proc(long id);

struct channel* create_channel(long id, long node1, long node2, uint32_t latency);

struct channel* create_channel_post_proc(long id, long direction1, long direction2, long node1, long node2, uint64_t capacity, uint32_t latency);

struct edge* create_edge(long id, long channel_id, long counterparty, struct policy policy);

struct edge* create_edge_post_proc(long id, long channel_id, long other_direction, long counterparty, uint64_t balance, struct policy policy);

struct payment* create_payment(long id, long sender, long receiver, uint64_t amount);

void connect_nodes(long node1, long node2);

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

#endif
