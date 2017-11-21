#ifndef heap_h
#define heap_h

typedef struct element {
  void* data; 
} Element;

typedef struct heap {
  long size;
  long index;
  void** data;
} Heap;


void initialize(Heap* h, long size);

void insert(Heap *h, void* data, int(*compare)());

void* pop(Heap* h, int(*compare)());

#endif
