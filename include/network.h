#ifndef NETWORK_H
#define NETWORK_H

#include <stdio.h>
#include <stdint.h>
#include "cloth.h"
#include "list.h"

#define MAXMSATOSHI 5E17 //5 millions  bitcoin
#define MAXTIMELOCK 100
#define MINTIMELOCK 10
#define MAXFEEBASE 5000
#define MINFEEBASE 1000
#define MAXFEEPROP 10
#define MINFEEPROP 1
#define MAXLATENCY 100
#define MINLATENCY 10
#define MINBALANCE 1E2
#define MAXBALANCE 1E11


struct policy {
  uint64_t fee_base;
  uint64_t fee_proportional;
  uint64_t min_htlc;
  uint32_t timelock;
};

struct ignored{
  long id;
  uint64_t time;
};

/* struct random_network_node{ */
/*   long id; */
/*   int n_channels; */
/*   int channel_count; */
/*   unsigned int in_network; */
/*   unsigned int left; */
/*   struct element* connections; */
/* }; */

struct graph_channel {
  long node1_id;
  long node2_id;
};

struct node_pair_result{
  long to_node_id;
  uint64_t fail_time;
  uint64_t fail_amount;
  uint64_t success_time;
  uint64_t success_amount;
};

struct node {
  long id;
  struct array* open_edges;
  struct element **results;
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
  long from_node_id;
  long to_node_id;
  long counter_edge_id;
  struct policy policy;
  uint64_t balance;
  unsigned int is_closed;
  uint64_t tot_flows;
};

struct network {
  struct array* nodes;
  struct array* channels;
  struct array* edges;
  gsl_ran_discrete_t* faulty_node_prob;
};


struct node* new_node(long id);

struct channel* new_channel(long id, long direction1, long direction2, long node1, long node2, uint64_t capacity, uint32_t latency);

struct edge* new_edge(long id, long channel_id, long counter_edge_id, long from_node_id, long to_node_id, uint64_t balance, struct policy policy);

void open_channel(struct network* network, gsl_rng* random_generator);

struct network* initialize_network(struct network_params net_params, gsl_rng* random_generator);


#endif
