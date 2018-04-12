#include <stdio.h>
#include <stdlib.h>
#include "array.h"
//#include "../gc-7.2/include/gc.h"

Array* resizeArray(Array* a) {
  Array* new;
  long i;

  new=malloc(sizeof(Array));
  new->size = a->size*2;
  new->index = a->index;
  new->element = malloc(new->size*sizeof(void*));
  for(i=0; i<new->index; i++)
    new->element[i]=a->element[i];
  for(;i<new->size;i++)
    new->element[i] = NULL;

  return new;
}

Array* arrayInitialize(long size) {
  Array* a;

  a = malloc(sizeof(Array));
  a->size = size;
  a->index = 0;
  a->element = malloc(a->size*sizeof(void*));

  return a;
}


Array* arrayInsert(Array* a, void* data) {
  if(a->index >= a->size)
    a = resizeArray(a);

  a->element[a->index]=data;
  (a->index)++;

  return a;
}

void* arrayGet(Array* a,long i) {
  if(i>=a->size || i>=a->index) return NULL;
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

void deleteElement(Array *a, long elementIndex) {
  long i;

  (a->index)--;

  for(i = elementIndex; i < a->index ; i++)
    a->element[i] = a->element[i+1];
}

void arrayDelete(Array* a, void* element,  int(*isEqual)()) {
  long i;

  for(i = 0; i < arrayLen(a); i++) {
    if(isEqual(a->element[i], element))
      deleteElement(a, i);
  }
}

void arrayDeleteAll(Array* a) {
  a->index = 0;
}
