#ifndef heap_h
#define heap_h


struct heap {
  long size;
  long index;
  void** data;
};


struct heap* heap_initialize(long size);

struct heap* heap_insert(struct heap *h, void* data, int(*compare)());

void* heap_pop(struct heap* h, int(*compare)());

long heap_len(struct heap*h);

void heap_free(struct heap* h);

#endif
