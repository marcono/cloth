#ifndef ARRAY_H
#define ARRAY_H

typedef struct array {
  void **element;
  long size;
  long index;
} Array;

Array* array_initialize(long size);

Array*  array_insert(Array* a, void* data);

void* array_get(Array* a,long i);

long array_len(Array* a);

void array_reverse(Array* a);

void array_delete(Array* a, void* element,  int(*is_equal)());

void array_delete_all(Array* a);

#endif
