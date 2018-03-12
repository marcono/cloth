#ifndef list_h
#define list_h


typedef struct node {
	struct node* next;
	long data;
} Node;

Node* push(Node* head, long data);

Node* pop(Node* head, long* data);

#endif
