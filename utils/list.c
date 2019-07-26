#include <stdio.h>
#include <stdlib.h>

#include "list.h"

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
