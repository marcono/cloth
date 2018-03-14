#ifndef HASHTABLE_H
#define HASHTABLE_H

typedef struct element {
	long key;
  void *value;
	struct element *next;
} Element;

typedef struct {
  Element **table;
  int size;
} HashTable;

HashTable* hashTableInitialize(int size);

void hashTablePut(HashTable* ht, long key, void *val);

void* hashTableGet(HashTable *ht, long key);

void hashTableMatrixPut(HashTable* ht, long i, long j, void* val);

void* hashTableMatrixGet(HashTable* ht, long i, long j);

void hashTableCountElements(HashTable* ht);

#endif
