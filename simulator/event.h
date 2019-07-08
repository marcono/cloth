#ifndef EVENT_H
#define EVENT_H
#include "../utils/heap.h"
#include <stdint.h>

/*
typedef struct payment {
  Peer sender;
  Peer receiver;
  double amount;
  List route;
} Payment;
*/

extern long event_index;
extern Heap* events;

typedef enum event_type {
  FINDROUTE,
  SENDPAYMENT,
  FORWARDPAYMENT,
  RECEIVEPAYMENT,
  FORWARDSUCCESS,
  FORWARDFAIL,
  RECEIVESUCCESS,
  RECEIVEFAIL
} Event_type;

typedef struct event {
  long ID;
  uint64_t time;
  Event_type type;
  long peer_iD;
  long payment_iD;
} Event;

Event* create_event(long ID, uint64_t time, Event_type type, long peer_iD, long payment_iD);

int compare_event(Event* e1, Event *e2);

void print_event(Event*e);


#endif
