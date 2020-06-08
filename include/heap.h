#ifndef heap_h
#define heap_h


struct heap {
  long size;
  long index;
  void** data;
};


struct heap* heap_initialize(long size);

struct heap* heap_insert(struct heap *h, void* data, int(*compare)());

struct heap* heap_insert_or_update(struct heap *h, void* data, int(*compare)(), int(*is_key_equal)());

void* heap_pop(struct heap* h, int(*compare)());

long heap_len(struct heap*h);

void heap_free(struct heap* h);

#endif
