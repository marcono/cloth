#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../gc-7.2/include/gc.h"
#include "../utils/hashTable.h"
#include "../protocol/protocol.h"

void initialize() {
  HashTable* nodes, *edges;
  long i;
  Node* node;
  Edge* edge;
  long edgeSize=2;

  nodes = initializeHashTable(2);
  edges = initializeHashTable(2);

  for(i=0; i<5; i++) {
    node = initializeNode(i, edgeSize);
    put(nodes, node->ID, node);
  }


  long j, edgeID, counterpartyID, edgeIndex;
  Node* counterparty;
  srand(time(NULL));
  for(i=0; i<5; i++) {
    for(j=0; j<edgeSize; j++){
      node = get(nodes, i);
      if(node->edge[j]!=-1) continue;

 
      do {
        counterpartyID = rand()%5;
      }while(counterpartyID==node->ID);

      counterparty = get(nodes, counterpartyID);
      edgeIndex = getEdgeIndex(counterparty);
      if(edgeIndex==-1) continue;

      edgeID = rand()%100;
      edge=createEdge(edgeID, counterpartyID, 0.0  );
      put(edges, edge->ID, edge);
      node->edge[j]=edge->ID;
      counterparty->edge[edgeIndex]=edge->ID;
    } 
  }

  for(i=0; i<5; i++) {
    node = get(nodes, i);
    for(j=0; j<edgeSize; j++) {
      if(node->edge[j]==-1) continue;
      edge = get(edges, node->edge[j]);
      printf("Node %ld is connected to node %ld through edge %ld", i, edge->ID, edge->counterparty);
    } 
  }

}

