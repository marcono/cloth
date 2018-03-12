#include <stdio.h>
#include <stdlib.h>

#include "list.h"

Node* push(Node* head, long data) {
	Node* newhead;

	newhead = malloc(sizeof(Node));
	newhead->data = data;
  newhead->next = head;

	return newhead;
}

Node* pop(Node* head, long* data) {
  if(head==NULL) {
    *data = -1;
    return NULL;
  }
  *data = head->data;
  head = head->next;
  return head;
}
