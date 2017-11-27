#ifndef ARRAY_H
#define ARRAY_H

typedef struct array {
  long *element;
  long size;
  long index;
} Array;

Array* arrayInitialize(long size);

void arrayInsert(Array* a, long data);

long arrayGet(Array* a,long i);

long arrayLen(Array* a);

void arrayReverse(Array* a);

#endif
