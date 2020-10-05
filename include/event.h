#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>
#include "heap.h"
#include "array.h"
#include "payments.h"

enum event_type {
  FINDPATH,
  SENDPAYMENT,
  FORWARDPAYMENT,
  RECEIVEPAYMENT,
  FORWARDSUCCESS,
  FORWARDFAIL,
  RECEIVESUCCESS,
  RECEIVEFAIL,
  OPENCHANNEL
};

struct event {
  uint64_t time;
  enum event_type type;
  long node_id;
  struct payment *payment;
};

struct event* new_event(uint64_t time, enum event_type type, long node_id, struct payment* payment);

int compare_event(struct event* e1, struct event *e2);

struct heap* initialize_events(struct array* payments);

#endif
