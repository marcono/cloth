#ifndef NETWORK_H
#define NETWORK_H

#include <stdio.h>
#include <stdint.h>
#include "cloth.h"

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
#define HOPSLIMIT 20


extern long edge_index, node_index, channel_index;
FILE *csv_node, *csv_edge, *csv_channel;
extern gsl_ran_discrete_t* uncoop_before_discrete, *uncoop_after_discrete;

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

extern struct array* nodes;
extern struct array* edges;
extern struct array* channels;


struct node* new_node(long id);

struct channel* new_channel(long id, long direction1, long direction2, long node1, long node2, uint64_t capacity, uint32_t latency);

struct edge* new_edge(long id, long channel_id, long other_direction, long counterparty, uint64_t balance, struct policy policy);

void initialize_network(struct network_params net_params, unsigned int is_preproc);

void generate_network_from_file(unsigned int is_preproc);


#endif
