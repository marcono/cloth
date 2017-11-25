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

HashTable* initializeHashTable(int dim);

void put(HashTable* ht, long key, void *val);

void* get(HashTable *ht, long key);

#endif
