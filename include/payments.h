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

/* register an eventual error occurred when the payment traversed a hop */
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
  uint64_t start_time;
  uint64_t end_time;
  int attempts;
  struct payment_error error;
  /* attributes for multi-path-payment (mpp)*/
  unsigned int is_shard;
  long shards_id[2];
  /* attributes used for computing stats */
  unsigned int is_success;
  int offline_node_count;
  int no_balance_count;
  unsigned int is_timeout;

};

struct payment* new_payment(long id, long sender, long receiver, uint64_t amount, uint64_t start_time);
struct array* initialize_payments(struct payments_params pay_params, long n_nodes, gsl_rng* random_generator);

#endif
