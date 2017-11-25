#ifndef ARRAY_H
#define ARRAY_H

typedef struct array {
  long *element;
  long size;
  long index;
} Array;

Array* initializeArray(long size);

void insert (Array* a, long data);

long get(Array* a,long i);



#endif
