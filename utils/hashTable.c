#include <stdio.h>
#include <stdlib.h>
#include "../gc-7.2/include/gc.h"
#include "hashTable.h"
#define MAXLEN 64
#define NUMLISTE 1024


/**************************/


Element *lput(long key, void *value, Element *list) {
		Element *first;
		first = (Element *) GC_MALLOC(sizeof(Element));
		first->key= key;
		first->value= value;
		first->next = list;
		return first;
	}

//funzione che, in una list, restituisce il valore del nodo corrispondente alla chiave
void* lget(long key, Element *list) {
	if (list == NULL)
		return NULL;
	if (key== list->key)
		return list->value;
	return lget(key, list->next);
}


/********************************/

HashTable* initializeHashTable(long dim) {
	long i;
	HashTable* ht;
  ht = GC_MALLOC(sizeof(HashTable));
	ht->dim = dim;
	ht->table = (Element **) GC_MALLOC(dim*sizeof(Element *));

	for(i=0; i < dim; i++)
		ht->table[i] = NULL;
	return ht;
}


//funzione di hashing, che fa uso del finto generatore di numeri casuali (infatti srand è inizializzato a zero) e dello xor
unsigned long hashv(long i) {
	unsigned long hval=0;
	srand(0);

	
	hval = i ^ rand();
	

	return hval;
}

//funzione che inserice nella hashtable; l'indice del vettore si trova facendo il modulo dell'hashing value
void put(HashTable* ht, long key, void *val) {
	unsigned long index;
	index = hashv(key) % ht->dim;
 printf("%ld => %ld\n", key, index);
	ht->table[index] = lput(key, val, ht->table[index]);
}

//funzione che, nella hash table, restituisce il valore del nodo corrispondente a key
void* get(HashTable *ht, long key) {
	unsigned long index;
	index = hashv(key) % ht->dim;
    return lget(key, ht->table[index]);
}


