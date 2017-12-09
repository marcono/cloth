#include <stdio.h>
#include <string.h>
#include "../gc-7.2/include/gc.h"
#include "event.h"

Event* createEvent(long ID, double time, char type[20], long peerID, long paymentID) {
  Event* e;

  e = GC_MALLOC(sizeof(Event));
  e->ID = ID;
  e->time = time;
  strcpy(e->type, type);
  e->peerID = peerID;
  e->paymentID = paymentID;
  
  return e;

}


int compareEvent(Event *e1, Event *e2) {
  double time1, time2;

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
  printf("%ld, %lf\n", e->ID, e->time);
}
