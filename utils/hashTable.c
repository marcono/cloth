#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
//#include "../gc-7.2/include/gc.h"
#include "hashTable.h"
#include "../global.h"


/**************************/

 
Element *lput(long key, void *value, Element *list) {
		Element *first;
		first = (Element *) malloc(sizeof(Element));
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

void* lgetnew(long key, Element* list) {
  Element* node;

  for(node = list; node != NULL; node = node->next) {
    if(node->key == key) {
      return node->value;
    }
  }



  return NULL;
}

Element *lupdate(long key, void* value, Element* list) {
  Element* node;

  for(node = list; node != NULL; node = node->next) {
    if(node->key == key){
      node->value = value;
      return list;
    }
  }

  return lput(key, value, list);
}

long lgetsize(Element* list) {
  long size =0;
  Element* node;

  for(node = list; node != NULL; node = node->next) {
    size++;
  }

  return size;
}


/********************************/

HashTable* hashTableInitialize(int size) {
	int i;
	HashTable* ht;
  ht = malloc(sizeof(HashTable));
	ht->size = size;
	ht->table =  malloc(size*sizeof(Element *));

	for(i=0; i < size; i++)
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
	index = hashv(key) % ht->size;
	ht->table[index] = lput(key, val, ht->table[index]);
}

void* hashTableGet(HashTable *ht, long key) {
	unsigned long index;
	index = hashv(key) % ht->size;
  return lgetnew(key, ht->table[index]);
}

void hashTableMatrixPut(HashTable* ht, long i, long j, void* val){
  ht->table[i] = lput(j, val, ht->table[i]);
}

void* hashTableMatrixGet(HashTable* ht, long i, long j){
  return lget(j, ht->table[i]);
}

void hashTableCountElements(HashTable *ht) {
  long i;

  for(i=0; i<ht->size; i++) {
    printf("%ld\n", lgetsize(ht->table[i]));
  }
}


