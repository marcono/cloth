#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>


#include "./simulator/event.h"
#include "./simulator/initialize.h"
#include "./utils/heap.h"
#include "./utils/hashTable.h"
#include "./gc-7.2/include/gc.h"
#include "./utils/array.h"
#include "./protocol/findRoute.h"
#include "./protocol/protocol.h"


/*
// Test Yen

HashTable* peers, *channels, *channelInfos;
long nPeers, nChannels;

int main() {
  Array *paths;
  Array* path;
  Hop* hop;
  long i, j;
  Peer* peer;
  Channel* channel;
  ChannelInfo * channelInfo;

  peers = hashTableInitialize(2);
  channels = hashTableInitialize(2);
  channelInfos= hashTableInitialize(2);

  nPeers=8;

  

  for(i=0; i<nPeers; i++) {
    peer = createPeer(5);
    hashTablePut(peers, peer->ID, peer);
  }

  Policy policy;
  policy.fee=0.0;
  policy.timelock=1.0;

  for(i=1; i<5; i++) {
    connectPeers(i-1, i);
  }

  connectPeers(0, 5);
  connectPeers(5,6);
  connectPeers(6,4);

  connectPeers(0,7);
  connectPeers(7,4);




  long *currChannelID;
  for(i=0; i<nPeers; i++) {
    peer = hashTableGet(peers, i);
    //    printf("%ld ", arrayLen(peer->channel));
    for(j=0; j<arrayLen(peer->channel); j++) {
      currChannelID=arrayGet(peer->channel, j);
      if(currChannelID==NULL) {
        printf("null\n");
        continue;
      }
      channel = hashTableGet(channels, *currChannelID);
      printf("Peer %ld is connected to peer %ld through channel %ld\n", i, channel->counterparty, channel->ID);
      }
  }


    //initialize();
  printf("\nYen\n");
  paths=findPaths(0, 4, 0.0);
  printf("%ld\n", arrayLen(paths));
  for(i=0; i<arrayLen(paths); i++) {
    printf("\n");
     path = arrayGet(paths, i);
     for(j=0;j<arrayLen(path); j++) {
       hop = arrayGet(path, j);
       printf("(Channel, Peer) = (%ld, %ld) ", hop->channel, hop->peer);
     }

  return 0;
}

*/


//test dijkstra
int main() {
  Array *hops;
  PathHop* hop;
  Array *ignored;
  long i, sender, receiver;
  long fakeIgnored = -1;

  ignored = arrayInitialize(1);
  ignored = arrayInsert(ignored, &fakeIgnored);

  initialize();
  printf("\nDijkstra\n");

  sender = 4;
  receiver = 3;
  hops=dijkstra(sender, receiver, 0.0, ignored, ignored );
  printf("From node %ld to node %ld\n", sender, receiver);
  for(i=0; i<arrayLen(hops); i++) {
    hop = arrayGet(hops, i);
    printf("(Sender, Receiver, Channel) = (%ld, %ld, %ld) ", hop->sender, hop->receiver, hop->channel);
  }
  printf("\n\n");

  sender = 0;
  receiver = 1;
  hops=dijkstra(sender, receiver, 0.0, ignored, ignored );
  printf("From node %ld to node %ld\n", sender, receiver);
  for(i=0; i<arrayLen(hops); i++) {
    hop = arrayGet(hops, i);
    printf("(Sender, Receiver, Channel) = (%ld, %ld, %ld) ", hop->sender, hop->receiver, hop->channel);
  }
  printf("\n");

  sender = 0;
  receiver = 0;
  hops=dijkstra(sender,receiver, 0.0, ignored, ignored );
  printf("From node %ld to node %ld\n", sender, receiver);
  if(hops != NULL) {
    for(i=0; i<arrayLen(hops); i++) {
      hop = arrayGet(hops, i);
      printf("(Sender, Receiver, Channel) = (%ld, %ld, %ld) ", hop->sender, hop->receiver, hop->channel);
    }
  }
  printf("\n");


  return 0;
}


/*
int main() {
  Array *a;
  long i, *data;

  a=arrayInitialize(10);

  for(i=0; i< 21; i++){
    data = GC_MALLOC(sizeof(long));
    *data = i;
   a = arrayInsert(a,data);
  }

  for(i=0; i<21; i++) {
    data = arrayGet(a,i);
    if(data!=NULL) printf("%ld ", *data);
    else printf("null ");
  }

  printf("\n%ld ", a->size);

  return 0;
}*/

/*
int main() {
  HashTable *ht;
  long i;
  Event *e;
  long N=100000;

  ht = initializeHashTable(10);

  for(i=0; i<N; i++) {
    e = GC_MALLOC(sizeof(Event));
    e->time=0.0;
    e->ID=i;
    strcpy(e->type, "Send");

    put(ht, e->ID, e);

}

  long listDim[10]={0};
  Element* el;
  for(i=0; i<10;i++) {
    el =ht->table[i];
    while(el!=NULL){
      listDim[i]++;
      el = el->next;
    }
  }

  for(i=0; i<10; i++)
   printf("%ld ", listDim[i]);

  printf("\n");

  return 0;
}


/*
int main() {

  const gsl_rng_type * T;
  gsl_rng * r;

  int i, n = 10;
  double mu = 0.1;


  gsl_rng_env_setup();

  T = gsl_rng_default;
  r = gsl_rng_alloc (T);


  for (i = 0; i < n; i++)
    {
      double  k = gsl_ran_exponential (r, mu);
      printf (" %lf", k);
    }

  printf ("\n");
  gsl_rng_free (r);

  return 0;
   }
*/
