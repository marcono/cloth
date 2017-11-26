#include <stdlib.h>
#include "array.h"
#include "../gc-7.2/include/gc.h"

Array* resize(Array* a) {
  Array* new;
  long i;
  new=GC_MALLOC(sizeof(Array));
  new->size = a->size*2;
  new->index = a->index;
  new->element = GC_MALLOC(new->size*sizeof(long));
  for(i=0; i<new->index; i++)
    new->element[i]=a->element[i];
  for(;i<new->size;i++)
    new->element[i] = -1;
  return new;
}

Array* arrayInitialize(long size) {
  Array* a;
  long i;
  a = GC_MALLOC(sizeof(Array));
  a->size = size;
  a->index = 0;
  a->element = GC_MALLOC(a->size*sizeof(long));
  for(i=0; i<a->size; i++)
    a->element[i]=-1;
  return a;
}


void arrayInsert(Array* a, long data) {
  if(a->index >= a->size)
    a = resize(a);

  a->element[a->index]=data;
  (a->index)++;
}

long arrayGet(Array* a,long i) {
  if(i>=a->size) return -1;
  return a->element[i];
}

long arrayGetNElems(Array *a) {
  long nElems=0, i;
  for(i=0; i<a->index; i++)
    if(a->element[i]!=-1) ++nElems;
  return nElems;
}


