#ifndef CLOTH_H
#define CLOTH_H

#include <stdint.h>
#include "heap.h"
#include "gsl_rng.h"
#include "gsl_randist.h"


struct network_params{
  long n_nodes;
  long n_channels;
  double faulty_node_prob;
  double p_uncoop_after_lock;
  double RWithholding;
  int gini;
  int sigma_topology;
  long capacity_per_channel;
};

struct payments_params{
  double payment_mean;
  long n_payments;
  double sigma_amount;
  double same_destination;
};

struct simulation{
  uint64_t current_time; //milliseconds
  struct heap* events;
  gsl_rng* random_generator;
};

#endif
