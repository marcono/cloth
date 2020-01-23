#ifndef INITIALIZE_H
#define INITIALIZE_H

#include <stdint.h>
#include "gsl_rng.h"
#include "array.h"
#include "heap.h"
#include "cloth.h"

extern uint64_t simulator_time; //milliseconds
FILE* csv_payment;
extern struct array* payments;
extern long payment_index;

extern long event_index;
extern struct heap* events;

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

enum event_type {
  FINDROUTE,
  SENDPAYMENT,
  FORWARDPAYMENT,
  RECEIVEPAYMENT,
  FORWARDSUCCESS,
  FORWARDFAIL,
  RECEIVESUCCESS,
  RECEIVEFAIL
};

struct event {
  long id;
  uint64_t time;
  enum event_type type;
  long node_id;
  long payment_id;
};

struct event* new_event(long id, uint64_t time, enum event_type type, long node_id, long payment_id);

int compare_event(struct event* e1, struct event *e2);

struct payment* new_payment(long id, long sender, long receiver, uint64_t amount);

void initialize_payments(struct payments_params pay_params, unsigned int is_preproc);

void generate_payments_from_file(unsigned int is_preproc);

#endif
