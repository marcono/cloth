#ifndef list_h
#define list_h


struct element {
	struct element* next;
	void* data;
};

struct element* push(struct element* head, void* data);

void* get_by_key(struct element* head, long key, int (*is_key_equal)());

struct element* pop(struct element* head, void** data);

long list_len(struct element* head);

unsigned int is_in_list(struct element* head, void* data, int(*is_equal)());

void list_free(struct element* head);

#endif
