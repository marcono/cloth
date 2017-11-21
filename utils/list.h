#ifndef list_h
#define list_h


typedef struct node {
	struct node* next;
	int data;
} Node;

Node* push(Node* head, int data);

#endif
