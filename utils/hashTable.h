#ifndef HASHTABLE_H
#define HASHTABLE_H

typedef struct element {
	long key;
  void *value;
	struct element *next;
} Element;

typedef struct {
  Element **table;
  int dim;
} HashTable;

HashTable* hashTableInitialize(int dim);

void hashTablePut(HashTable* ht, long key, void *val);

void* hashTableGet(HashTable *ht, long key);

#endif
