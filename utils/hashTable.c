#include <stdio.h>
#include <stdlib.h>
#include "../gc-7.2/include/gc.h"
#include "hashTable.h"




/**************************/

Element *lput(long key, void *value, Element *list) {
		Element *first;
		first = (Element *) GC_MALLOC(sizeof(Element));
		first->key= key;
		first->value= value;
		first->next = list;
		return first;
	}

void* lget(long key, Element *list) {
	if (list == NULL)
		return NULL;
	if (key== list->key)
		return list->value;
	return lget(key, list->next);
}


/********************************/

HashTable* hashTableInitialize(int dim) {
	int i;
	HashTable* ht;
  ht = GC_MALLOC(sizeof(HashTable));
	ht->dim = dim;
	ht->table = (Element **) GC_MALLOC(dim*sizeof(Element *));

	for(i=0; i < dim; i++)
		ht->table[i] = NULL;
	return ht;
}

unsigned long hashv(long i) {
	unsigned long hval=0;

  srand(0);
	hval = i ^ rand();

	return hval;
}

void hashTablePut(HashTable* ht, long key, void *val) {
	unsigned long index;
	index = hashv(key) % ht->dim;
	ht->table[index] = lput(key, val, ht->table[index]);
}

void* hashTableGet(HashTable *ht, long key) {
	unsigned long index;
	index = hashv(key) % ht->dim;
    return lget(key, ht->table[index]);
}


