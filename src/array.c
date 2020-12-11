#include <stdio.h>
#include <stdlib.h>
#include "../include/array.h"

struct array* resize_array(struct array* a) {
  struct array* new;
  long i;

  new=malloc(sizeof(struct array));
  new->size = a->size*2;
  new->index = a->index;
  new->element = malloc(new->size*sizeof(void*));
  for(i=0; i<new->index; i++)
    new->element[i]=a->element[i];
  for(;i<new->size;i++)
    new->element[i] = NULL;

  return new;
}

struct array* array_initialize(long size) {
  struct array* a;

  a = malloc(sizeof(struct array));
  a->size = size;
  a->index = 0;
  a->element = malloc(a->size*sizeof(void*));

  return a;
}


struct array* array_insert(struct array* a, void* data) {
  if(a->index >= a->size)
    a = resize_array(a);

  a->element[a->index]=data;
  (a->index)++;

  return a;
}

void* array_get(struct array* a,long i) {
  if(i>=a->size || i>=a->index) return NULL;
  return a->element[i];
}

long array_len(struct array *a) {
  return a->index;
}

void array_reverse(struct array *a) {
  long i, n;
  void*tmp;

  n = array_len(a);

  for(i=0; i<n/2; i++) {
    tmp = a->element[i];
    a->element[i] = a->element[n-i-1];
    a->element[n-i-1] = tmp;
  }
}

void delete_element(struct array *a, long element_index) {
  long i;

  (a->index)--;

  for(i = element_index; i < a->index ; i++)
    a->element[i] = a->element[i+1];
}

void array_delete(struct array* a, void* element,  int(*is_equal)()) {
  long i;

  for(i = 0; i < array_len(a); i++) {
    if(is_equal(a->element[i], element))
      delete_element(a, i);
  }
}

void array_delete_all(struct array* a) {
  a->index = 0;
}

void array_free(struct array* a)  {
  free(a->element);
  free(a);
}
