#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "../include/event.h"

struct event* create_event(long id, uint64_t time, enum event_type type, long node_id, long payment_id) {
  struct event* e;

  e = malloc(sizeof(struct event));
  e->id = id;
  e->time = time;
  e->type = type;
  e->node_id = node_id;
  e->payment_id = payment_id;

  event_index++;

  return e;

}


int compare_event(struct event *e1, struct event *e2) {
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


void print_event(struct event* e){
  printf("%ld, %ld\n", e->id, e->time);
}
