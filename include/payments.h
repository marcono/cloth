#ifndef PAYMENTS_H
#define PAYMENTS_H

#include <stdint.h>
#include <gsl/gsl_rng.h>
#include "array.h"
#include "heap.h"
#include "cloth.h"
#include "network.h"


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


struct array* initialize_payments(struct payments_params pay_params, long n_nodes, gsl_rng* random_generator);

#endif
