#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "./simulator/event.h"
#include "./utils/heap.h"
#include "./gc-7.2/include/gc.h"


int compareInt(int* a, int* b) {
  if (*a==*b)
    return 0;
  else if(*a<*b)
    return -1;
  else
    return 1;
}

void printInt(int *a) {
  printf("%d\n",*a );
}

int main() {
  long N=10;
  int times[]={15, 15, 96, 7, 1, 14, 7, 18, 3, 82};
  Event *e;
  long i;
  Heap* h;

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

  /*	Node* root=NULL;
 long  i=0;
  long N=100000;
  double time=0.0;
  Event *e;

	for(i=0; i<N; i++){
    e = malloc(sizeof(Event));
    e->ID=i;
    e->time=time;
    strcpy(e->type,"Send");
		root=push(root, e, compareEvent);
    time+=0.001;
	}

	printTree(root, printEvent);

	//printf("\n\n%d\n\n", len(root));


	//for(i=0; i<5; i++) {
  //root=pop(root,&e);		
  //printEvent(e);
	//}

	//printf("\n\nTreeLen: %d\n\n", len(root));
  */
	return 0;
}
