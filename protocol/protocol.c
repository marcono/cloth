#include <stdlib.h>
#include "../gc-7.2/include/gc.h"
#include "protocol.h"

Node* initializeNode(long ID, long edgeSize) {
  Node* node;
  long i;
  node = GC_MALLOC(sizeof(Node));
  node->ID=ID;
  node->edgeSize = edgeSize;
  node->edge=GC_MALLOC(node->edgeSize*sizeof(long));
  for(i=0; i<node->edgeSize; i++)
    node->edge[i]=-1;
  return node;
}

Edge* createEdge(long ID, long counterparty, double capacity){
  Edge* edge;
  edge = GC_MALLOC(sizeof(edge));
  edge->ID = ID;
  edge->counterparty = counterparty;
  edge->capacity = capacity;
  return edge;
}

long getEdgeIndex(Node* node) {
  long index=-1, i;
  for(i=0; i<node->edgeSize; i++) {
    if(node->edge[i]==-1) index=i;
  }
  return index;
}
