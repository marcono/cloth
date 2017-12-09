#ifndef EVENT_H
#define EVENT_H
/*
typedef struct payment {
  Peer sender;
  Peer receiver;
  double amount;
  List route;
} Payment;
*/

extern long eventID;

typedef struct event {
  long ID;
  double time;
  char type[20];
  long peerID;
  long paymentID;
} Event;

Event* createEvent(long ID, double time, char type[20], long peerID, long paymentID);

int compareEvent(Event* e1, Event *e2);

void printEvent(Event*e);

#endif
