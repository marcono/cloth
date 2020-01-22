#ifndef CLOTH_H
#define CLOTH_H

#include "gsl_rng.h"
#include "gsl_randist.h"

extern gsl_rng *random_generator;
extern const gsl_rng_type * random_generator_type;

struct network_params{
  long n_nodes;
  long n_channels;
  double p_uncoop_before_lock;
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

#endif
