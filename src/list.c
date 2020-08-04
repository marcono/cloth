#include <stdio.h>
#include <stdlib.h>
#include "../include/list.h"

struct element* push(struct element* head, void* data) {
	struct element* newhead;

	newhead = malloc(sizeof(struct element));
	newhead->data = data;
  newhead->next = head;

	return newhead;
}

void* get_by_key(struct element* head, long key, int (*is_key_equal)()){
  struct element* iterator;
  for(iterator=head; iterator!=NULL; iterator=iterator->next)
    if(is_key_equal(key, iterator->data))
      return iterator->data;
  return NULL;
}

struct element* pop(struct element* head, void** data) {
  if(head==NULL) {
    *data = NULL;
    return NULL;
  }
  *data = head->data;
  head = head->next;
  return head;
}

long list_len(struct element* head){
  long len;
  struct element* iterator;

  len=0;
  for(iterator=head; iterator!=NULL; iterator=iterator->next)
    ++len;

  return len;
}

unsigned int is_in_list(struct element* head, void* data, int (*is_equal)()){
  struct element* iterator;
  for(iterator=head; iterator!=NULL; iterator=iterator->next)
    if(is_equal(iterator->data, data))
      return 1;
  return 0;
}


void list_free(struct element* head){
  struct element* iterator, *next;
  for(iterator=head; iterator!=NULL;){
    next = iterator->next;
    free(iterator);
    iterator = next;
  }
}
