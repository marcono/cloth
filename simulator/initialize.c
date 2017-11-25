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

  nodes = hashTableInitialize(2);
  edges = hashTableInitialize(2);

  for(i=0; i<5; i++) {
    node = initializeNode(i, edgeSize);
    hashTablePut(nodes, node->ID, node);
  }


  long j, edgeID=-1, counterpartyID, edgeIndex;
  Node* counterparty;
  srand(time(NULL));
  for(i=0; i<5; i++) {
    node = hashTableGet(nodes, i);
    for(j=0; j<edgeSize; j++){
      if(node->edge[j]!=-1) continue;

 
      do {
        counterpartyID = rand()%5;
      }while(counterpartyID==node->ID);

      counterparty = hashTableGet(nodes, counterpartyID);
      edgeIndex = getEdgeIndex(counterparty);
      if(edgeIndex==-1) continue;

      edgeID++;
      edge=createEdge(edgeID, counterpartyID, 0.0  );
      hashTablePut(edges, edge->ID, edge);
      node->edge[j]=edge->ID;
      counterparty->edge[edgeIndex]=edge->ID;
    } 
  }

  for(i=0; i<5; i++) {
    node = hashTableGet(nodes, i);
    for(j=0; j<edgeSize; j++) {
      if(node->edge[j]==-1) continue;
      edge = hashTableGet(edges, node->edge[j]);
      printf("Node %ld is connected to node %ld through edge %ld\n", i, edge->counterparty, edge->ID);
    } 
  }

}

