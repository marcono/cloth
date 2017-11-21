#include <stdio.h>
#include <stdlib.h>

#include "list.h"

Node* push(Node* head, int data) {
	if (head==NULL) {
		head = malloc(sizeof(Node));
		head->data = data;
		head->next = NULL;
		return head;
	}
	
	Node* newhead;
	newhead = malloc(sizeof(Node));
	newhead->data = data;
  newhead->next = head;

	return newhead;
}
