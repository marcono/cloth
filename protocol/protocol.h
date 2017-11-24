#ifndef PROTOCOL_H
#define PROTOCOL_H

typedef struct policy {
  double fee;
  double timelock;
} Policy;

typedef struct node {
  long ID;
  long edgeSize;
  long* edge;
} Node;

typedef struct edge {
  long ID;
  long counterparty;
  double capacity;
  Policy policy;
} Edge;

typedef struct peer {
  long ID;
  long* node;
  long* channel;
} Peer;

typedef struct channel{
  long ID;
  long counterparty;
  double capacity;
  double balance;
  Policy policy;
} Channel;

Node* initializeNode(long ID, long edgeSize);

Edge* createEdge(long ID, long counterparty, double capacity);

long getEdgeIndex(Node*n);

#endif
