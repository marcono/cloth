#include <stdio.h>
#include <stdlib.h>
#include "../gc-7.2/include/gc.h"
#include "../simulator/initialize.h"
#include "../protocol/protocol.h"
#include "../utils/heap.h"
#include "../utils/array.h"
#define INF 10000000.0
#define HOPSLIMIT 20

typedef struct distance{
  long peer;
  double distance;
} Distance;

typedef struct previousPeer{
  long peer;
  long channel;
} PreviousPeer;

int compareDistance(Distance* a, Distance* b) {
  double d1, d2;
  d1=a->distance;
  d2=b->distance;
  if(d1==d2)
    return 0;
  else if(d1<d2)
    return -1;
  else
    return 1;
}


Array* dijkstra(long source, long target, double amount) {
  Distance distance[nPeers], *d;
  long i, bestPeerID, j, channelID, nextPeerID, prev;
  Heap *distanceHeap;
  Peer* bestPeer;
  Channel* channel;
  ChannelInfo* channelInfo;
  double tmpDist, capacity;
  PreviousPeer previousPeer[nPeers];
  Array* hops;

  distanceHeap = heapInitialize(10);

  for(i=0; i<nPeers; i++){
    distance[i].peer = i;
    distance[i].distance = INF;
    previousPeer[i].channel = -1;
    previousPeer[i].peer = -1;
  }

  distance[source].peer = source;
  distance[source].distance = 0.0;

  //TODO: e' safe passare l'inidrizzo dell'i-esimo elemento dell'array?
  heapInsert(distanceHeap, &distance[source], compareDistance); 

  while(heapLen(distanceHeap)!=0) {
    d = heapPop(distanceHeap, compareDistance);
    bestPeerID = d->peer;
    if(bestPeerID==target) break;

    bestPeer = hashTableGet(peers, bestPeerID);

    for(j=0; j<nChannels; j++) {
      channelID = arrayGet(bestPeer->channel, j);
      if(channelID==-1) continue;

      channel = hashTableGet(channels, channelID);
      nextPeerID = channel->counterparty;

      tmpDist = distance[bestPeerID].distance + channel->policy.timelock;

      //TODO: escludi blacklisted peer e channels


      channelInfo = hashTableGet(channelInfos, channel->channelInfoID);
      capacity = channelInfo->capacity;

      if(tmpDist < distance[nextPeerID].distance && amount<=capacity) {
        distance[nextPeerID].peer = nextPeerID;
        distance[nextPeerID].distance = tmpDist;

        previousPeer[nextPeerID].channel = channelID;
        previousPeer[nextPeerID].peer = bestPeerID;

        heapInsert(distanceHeap, &distance[nextPeerID], compareDistance);
      }
      }

    }

  if(previousPeer[target].peer == -1) {
    printf ("no path available!\n");
    return NULL;
  }

  hops=arrayInitialize(HOPSLIMIT);
  prev=target;
  while(prev!=source) {
    //    printf("%ld ", previousPeer[prev].peer);
    arrayInsert(hops, previousPeer[prev].channel);
    prev = previousPeer[prev].peer;
  }

  if(arrayLen(hops)>HOPSLIMIT)
    return NULL;

  arrayReverse(hops);

  return hops;
}
