#include <stdio.h>
#include "event.h"

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
