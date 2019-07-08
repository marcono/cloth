#include <stdio.h>
#include <stdlib.h>
#include "array.h"
//#include "../gc-7.2/include/gc.h"

Array* resize_array(Array* a) {
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

Array* array_initialize(long size) {
  Array* a;

  a = malloc(sizeof(Array));
  a->size = size;
  a->index = 0;
  a->element = malloc(a->size*sizeof(void*));

  return a;
}


Array* array_insert(Array* a, void* data) {
  if(a->index >= a->size)
    a = resize_array(a);

  a->element[a->index]=data;
  (a->index)++;

  return a;
}

void* array_get(Array* a,long i) {
  if(i>=a->size || i>=a->index) return NULL;
  return a->element[i];
}

long array_len(Array *a) {
  return a->index;
}

void array_reverse(Array *a) {
  long i, n;
  void*tmp;

  n = array_len(a);

  for(i=0; i<n/2; i++) {
    tmp = a->element[i];
    a->element[i] = a->element[n-i-1];
    a->element[n-i-1] = tmp;
  }
}

void delete_element(Array *a, long element_index) {
  long i;

  (a->index)--;

  for(i = element_index; i < a->index ; i++)
    a->element[i] = a->element[i+1];
}

void array_delete(Array* a, void* element,  int(*is_equal)()) {
  long i;

  for(i = 0; i < array_len(a); i++) {
    if(is_equal(a->element[i], element))
      delete_element(a, i);
  }
}

void array_delete_all(Array* a) {
  a->index = 0;
}
