#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
//#include "../gc-7.2/include/gc.h"
#include "event.h"

Event* createEvent(long ID, uint64_t time, EventType type, long peerID, long paymentID) {
  Event* e;

  e = malloc(sizeof(Event));
  e->ID = ID;
  e->time = time;
  e->type = type;
  e->peerID = peerID;
  e->paymentID = paymentID;

  eventIndex++;

  return e;

}


int compareEvent(Event *e1, Event *e2) {
  uint64_t time1, time2;

  time1=e1->time;
  time2=e2->time;

  if(time1==time2)
    return 0;
  else if(time1<time2)
    return -1;
  else
    return 1;
}


void printEvent(Event* e){
  printf("%ld, %ld\n", e->ID, e->time);
}
