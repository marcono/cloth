#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <gsl/gsl_sf_bessel.h>

#include "./simulator/event.h"
#include "./utils/heap.h"
#include "./gc-7.2/include/gc.h"


int main() {
  long N=10;
  int times[]={15, 15, 96, 7, 1, 14, 7, 18, 3, 82};
  Event *e;
  long i;
  Heap* h;
  int k=0;
  k=1;

  h = GC_MALLOC(sizeof(Heap));
  initialize(h, N);

  srand(time(NULL));
  for(i=0; i<N; i++){
    e = GC_MALLOC(sizeof(Event));
    e->ID=i;
    e->time=times[i];
    //    printf("%ld: %lf\n",e->ID, e->time);
    strcpy(e->type,"Send");
		insert(h, e, compareEvent);
	}
i=0;
  while((e=pop(h, compareEvent))!=NULL) {
    printf("%ld: %lf\n",e->ID,  e->time);
    i++;
  }

  printf("heap size: \n");

	return 0;
  }
