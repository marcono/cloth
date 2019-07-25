#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
//#include "../gc-7.2/include/gc.h"
#include "event.h"

Event* create_event(long ID, uint64_t time, Event_type type, long peer_id, long payment_id) {
  Event* e;

  e = malloc(sizeof(Event));
  e->ID = ID;
  e->time = time;
  e->type = type;
  e->peer_id = peer_id;
  e->payment_id = payment_id;

  event_index++;

  return e;

}


int compare_event(Event *e1, Event *e2) {
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


void print_event(Event* e){
  printf("%ld, %ld\n", e->ID, e->time);
}
