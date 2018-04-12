#include <stdio.h>
#include <stdlib.h>
#include "heap.h"
//#include "../gc-7.2/include/gc.h"

long getParent(long i) {
  return (i-1)/2;
}

long getLeftChild(long i) {
  return 2*i+1;
}


long getRightChild(long i) {
  return 2*i+2;
}

void swap(void** a, void** b) {
  void* tmp;
  tmp=*a;
  *a=*b;
  *b=tmp;
}

Heap* resizeHeap(Heap* h) {
  Heap* new;
  long i;
  new=malloc(sizeof(Heap));
  new->size = h->size*2;
  new->index = h->index;
  new->data = malloc(new->size*sizeof(void*));
  for(i=0; i<new->index; i++)
    new->data[i]=h->data[i];
  return new;
}

void heapify(Heap* h, long i, int(*compare)() ){
  long leftChild, rightChild, smallest;
  int compRes;

  smallest=i;
  leftChild = getLeftChild(smallest);
  rightChild = getRightChild(smallest);

  compRes = leftChild < h->index ? (*compare)(h->data[smallest], h->data[leftChild]) : -1;
  if(compRes>=0)
    smallest=leftChild;

  compRes = rightChild < h->index ? (*compare)(h->data[smallest], h->data[rightChild]) : -1;
  if(compRes>=0)
    smallest =rightChild;


  if(smallest!=i) {
    swap(&(h->data[i]),&(h->data[smallest]));
    heapify(h, smallest, compare);
  }
}


Heap* heapInitialize(long size) {
  Heap *h;
  h=malloc(sizeof(Heap));
  h->data = malloc(size*sizeof(void*));
  h->size = size;
  h->index = 0;
  return h;
}

Heap* heapInsert(Heap *h, void* data, int(*compare)()) {
  int i, parent, compRes;

  if(h->index>=h->size) {
    h = resizeHeap(h);
  }

  i=h->index;
  (h->index)++;
  h->data[i]=data;
  parent = getParent(i);

  while(i>0) {
    compRes=(*compare)(h->data[i], h->data[parent]);
    if(compRes>0) break;
    swap(&(h->data[i]),&(h->data[parent]));
    i=parent;
    parent=getParent(i);
  }

  return h;
}

void* heapPop(Heap* h, int(*compare)()) {
  void* min;

  if(h->index==0) return NULL;

  min = h->data[0];
  (h->index)--;
  h->data[0]=h->data[h->index];

  heapify(h, 0, compare);
  

  return min;
}

long heapLen(Heap*h){
  return h->index;
}

void heapFree(Heap *h) {
  long i;

  //  for(i=0; i<h->size; i++)
  // free(h->data[i]);

  free(h->data);
  free(h);
}

/*
  void heapify(Heap* h, int i, int(*compare)() ){
  int leftChild, rightChild, compResL, compResR;
  void* tmp;

  i=0;
  while(i<h->index) {

  leftChild = getLeftChild(i);
  rightChild = getRightChild(i);
  
  //  printEvent(h->data[i]);

  if(leftChild < (h->index))
  compResL = (*compare)(h->data[i], h->data[leftChild]);
  else
  compResL=-1;

  if(rightChild < (h->index))
  compResR = (*compare)(h->data[i], h->data[rightChild]);
  else
  compResR=-1;

  

  if(compResL>0) {
  tmp=h->data[i];
  h->data[i]=h->data[leftChild];
  h->data[leftChild]=tmp;
  i = leftChild;
  }
  if (compResR>0) {
  tmp=h->data[i];
  h->data[i]=h->data[rightChild];
  h->data[rightChild]=tmp;
  i = rightChild;
  }
  if(compResR<=0 && compResL<=0) break;

  }
  }*/

