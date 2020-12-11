#include <stdio.h>
#include <stdlib.h>

#include "../include/heap.h"

long get_parent(long i) {
  return (i-1)/2;
}

long get_left_child(long i) {
  return 2*i+1;
}


long get_right_child(long i) {
  return 2*i+2;
}

void swap(void** a, void** b) {
  void* tmp;
  tmp=*a;
  *a=*b;
  *b=tmp;
}

struct heap* resize_heap(struct heap* h) {
  struct heap* new;
  long i;
  new=malloc(sizeof(struct heap));
  new->size = h->size*2;
  new->index = h->index;
  new->data = malloc(new->size*sizeof(void*));
  for(i=0; i<new->index; i++)
    new->data[i]=h->data[i];
  return new;
}

void heapify(struct heap* h, long i, int(*compare)() ){
  long left_child, right_child, smallest;
  int comp_res;

  smallest=i;
  left_child = get_left_child(smallest);
  right_child = get_right_child(smallest);

  comp_res = left_child < h->index ? (*compare)(h->data[smallest], h->data[left_child]) : -1;
  if(comp_res>=0)
    smallest=left_child;

  comp_res = right_child < h->index ? (*compare)(h->data[smallest], h->data[right_child]) : -1;
  if(comp_res>=0)
    smallest =right_child;


  if(smallest!=i) {
    swap(&(h->data[i]),&(h->data[smallest]));
    heapify(h, smallest, compare);
  }
}


struct heap* heap_initialize(long size) {
  struct heap *h;
  h=malloc(sizeof(struct heap));
  h->data = malloc(size*sizeof(void*));
  h->size = size;
  h->index = 0;
  return h;
}


struct heap* heap_insert(struct heap *h, void* data, int(*compare)()) {
  long i, parent;
  int comp_res;

  if(h->index >= h->size) {
      h = resize_heap(h);
    }

  i=h->index;
  (h->index)++;
  h->data[i]=data;

  parent = get_parent(i);
  while(i>0) {
    comp_res=(*compare)(h->data[i], h->data[parent]);
    if(comp_res>0) break;
    swap(&(h->data[i]),&(h->data[parent]));
    i=parent;
    parent=get_parent(i);
  }

  return h;
}


struct heap* heap_insert_or_update(struct heap *h, void* data, int(*compare)(), int(*is_key_equal)()) {
  long i, parent;
  int comp_res, present;

  present = 0;
  for(i=0; i<h->index; i++){
    if(is_key_equal(h->data[i], data)){
      present = 1;
      break;
    }
  }

  if(!present){
    if(h->index>=h->size) {
      h = resize_heap(h);
    }
    i=h->index;
    (h->index)++;
    h->data[i]=data;
  }

  parent = get_parent(i);
  while(i>0) {
    comp_res=(*compare)(h->data[i], h->data[parent]);
    if(comp_res>0) break;
    swap(&(h->data[i]),&(h->data[parent]));
    i=parent;
    parent=get_parent(i);
  }

  return h;
}

void* heap_pop(struct heap* h, int(*compare)()) {
  void* min;

  if(h->index==0) return NULL;

  min = h->data[0];
  (h->index)--;
  h->data[0]=h->data[h->index];

  heapify(h, 0, compare);

  return min;
}

long heap_len(struct heap*h){
  return h->index;
}

void heap_free(struct heap *h) {
  free(h->data);
  free(h);
}

/*
  void heapify(struct heap* h, int i, int(*compare)() ){
  int left_child, right_child, comp_res_l, comp_res_r;
  void* tmp;

  i=0;
  while(i<h->index) {

  left_child = get_left_child(i);
  right_child = get_right_child(i);
  
  //  print_event(h->data[i]);

  if(left_child < (h->index))
  comp_res_l = (*compare)(h->data[i], h->data[left_child]);
  else
  comp_res_l=-1;

  if(right_child < (h->index))
  comp_res_r = (*compare)(h->data[i], h->data[right_child]);
  else
  comp_res_r=-1;

  

  if(comp_res_l>0) {
  tmp=h->data[i];
  h->data[i]=h->data[left_child];
  h->data[left_child]=tmp;
  i = left_child;
  }
  if (comp_res_r>0) {
  tmp=h->data[i];
  h->data[i]=h->data[right_child];
  h->data[right_child]=tmp;
  i = right_child;
  }
  if(comp_res_r<=0 && comp_res_l<=0) break;

  }
  }*/

