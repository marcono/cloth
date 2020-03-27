#include <stdio.h>
#include <stdlib.h>
#include "../include/list.h"

struct element* push(struct element* head, long data) {
	struct element* newhead;

	newhead = malloc(sizeof(struct element));
	newhead->data = data;
  newhead->next = head;

	return newhead;
}

struct element* pop(struct element* head, long* data) {
  if(head==NULL) {
    *data = -1;
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

unsigned int is_in_list(struct element* head, long value){
  struct element* iterator;
  for(iterator=head; iterator!=NULL; iterator=iterator->next)
    if(iterator->data==value)
      return 1;
  return 0;
}
