#ifndef heap_h
#define heap_h


typedef struct heap {
  long size;
  long index;
  void** data;
} Heap;


void initializeHeap(Heap* h, long size);

void insert(Heap *h, void* data, int(*compare)());

void* pop(Heap* h, int(*compare)());

#endif
