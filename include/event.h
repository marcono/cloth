#ifndef EVENT_H
#define EVENT_H
#include "heap.h"
#include <stdint.h>

/*
typedef struct payment {
  struct peer sender;
  struct peer receiver;
  double amount;
  List route;
} struct payment;
*/

extern long event_index;
extern struct heap* events;

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

struct event* create_event(long id, uint64_t time, enum event_type type, long node_id, long payment_id);

int compare_event(struct event* e1, struct event *e2);

void print_event(struct event*e);


#endif
