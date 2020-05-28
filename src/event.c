#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/event.h"
#include "../include/array.h"

struct event* new_event(uint64_t time, enum event_type type, long node_id, struct payment* payment) {
  struct event* e;

  e = malloc(sizeof(struct event));
  e->time = time;
  e->type = type;
  e->node_id = node_id;
  e->payment = payment;

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


struct heap* initialize_events(struct array* payments){
  struct heap* events;
  long i;
  struct event* event;
  struct payment* payment;
  uint64_t open_channel_time, last_payment_time;

  events = heap_initialize(array_len(payments)*10);

  for(i=0; i<array_len(payments); i++){
    payment = array_get(payments, i);
    event = new_event(payment->start_time, FINDROUTE, payment->sender, payment);
    events = heap_insert(events, event, compare_event);
  }

  /* payment = array_get(payments, array_len(payments)-1); */
  /* last_payment_time = payment->start_time; */
  /* for(open_channel_time=100; open_channel_time<last_payment_time; open_channel_time+=100){ */
  /*   event = new_event(open_channel_time, OPENCHANNEL, -1, NULL); */
  /*   events = heap_insert(events, event, compare_event); */
  /* } */

  return events;
}
