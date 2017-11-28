#include <stdlib.h>
#include "array.h"
#include "../gc-7.2/include/gc.h"

Array* resizeArray(Array* a) {
  Array* new;
  long i;

  new=GC_MALLOC(sizeof(Array));
  new->size = a->size*2;
  new->index = a->index;
  new->element = GC_MALLOC(new->size*sizeof(void*));
  for(i=0; i<new->index; i++)
    new->element[i]=a->element[i];
  for(;i<new->size;i++)
    new->element[i] = NULL;

  return new;
}

Array* arrayInitialize(long size) {
  Array* a;

  a = GC_MALLOC(sizeof(Array));
  a->size = size;
  a->index = 0;
  a->element = GC_MALLOC(a->size*sizeof(void*));

  return a;
}


void arrayInsert(Array* a, void* data) {
  if(a->index >= a->size)
    a = resizeArray(a);

  a->element[a->index]=data;
  (a->index)++;
}

void* arrayGet(Array* a,long i) {
  if(i>=a->size) return NULL;
  return a->element[i];
}

long arrayLen(Array *a) {
  return a->index;
}

void arrayReverse(Array *a) {
  long i, n;
  void*tmp;

  n = arrayLen(a);

  for(i=0; i<n/2; i++) {
    tmp = a->element[i];
    a->element[i] = a->element[n-i-1];
    a->element[n-i-1] = tmp;
  }
}
