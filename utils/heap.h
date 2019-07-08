#ifndef heap_h
#define heap_h


typedef struct heap {
  long size;
  long index;
  void** data;
} Heap;


Heap* heap_initialize(long size);

Heap* heap_insert(Heap *h, void* data, int(*compare)());

void* heap_pop(Heap* h, int(*compare)());

long heap_len(Heap*h);

void heap_free(Heap* h);

#endif
