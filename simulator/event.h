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

extern long eventIndex;
extern Heap* events;

typedef enum eventType {
  FINDROUTE,
  SENDPAYMENT,
  FORWARDPAYMENT,
  RECEIVEPAYMENT,
  FORWARDSUCCESS,
  FORWARDFAIL,
  RECEIVESUCCESS,
  RECEIVEFAIL
} EventType;

typedef struct event {
  long ID;
  uint64_t time;
  EventType type;
  long peerID;
  long paymentID;
} Event;

Event* createEvent(long ID, uint64_t time, EventType type, long peerID, long paymentID);

int compareEvent(Event* e1, Event *e2);

void printEvent(Event*e);


#endif
