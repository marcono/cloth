#ifndef PAYMENTS_H
#define PAYMENTS_H

#include <stdint.h>
#include <gsl/gsl_rng.h>
#include "array.h"
#include "heap.h"
#include "cloth.h"
#include "network.h"
#include "routing.h"

enum payment_error_type{
  NOERROR,
  NOBALANCE,
  OFFLINENODE, //it corresponds to `FailUnknownNextPeer` in lnd
};

struct payment_error{
  enum payment_error_type type;
  struct route_hop* hop;
};

struct payment {
  long id;
  long sender;
  long receiver;
  uint64_t amount; //millisatoshis
  struct route* route;
  unsigned int is_success;
  int offline_node_count;
  int no_balance_count;
  unsigned int is_timeout;
  uint64_t start_time;
  uint64_t end_time;
  int attempts;
  struct payment_error error;
};


struct array* initialize_payments(struct payments_params pay_params, long n_nodes, gsl_rng* random_generator);

#endif
