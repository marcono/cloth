#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../gc-7.2/include/gc.h"
#include "../simulator/initialize.h"
#include "../protocol/protocol.h"
#include "../utils/heap.h"
#include "../utils/array.h"
#include "findRoute.h"
#define INF 10000000.0
#define HOPSLIMIT 20

//FIXME: non globale ma passato per riferimento a dijkstra
char error[100];

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

int isPresent(long element, Array* longArray) {
  long i, *curr;
  for(i=0; i<arrayLen(longArray); i++) {
    curr = arrayGet(longArray, i);
    if(*curr==element) return 1;
  }
  return 0;
}

Array* dijkstra(long source, long target, double amount, Array* ignoredPeers, Array* ignoredChannels) {
  Distance distance[nPeers], *d;
  long i, bestPeerID, j,*channelID, nextPeerID, prev;
  Heap *distanceHeap;
  Peer* bestPeer;
  Channel* channel;
  ChannelInfo* channelInfo;
  double tmpDist, capacity;
  DijkstraHop previousPeer[nPeers];
  Array* hops;
  PathHop* hop;

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

    for(j=0; j<arrayLen(bestPeer->channel); j++) {
      channelID = arrayGet(bestPeer->channel, j);
      if(channelID==NULL) continue;

      channel = hashTableGet(channels, *channelID);
      nextPeerID = channel->counterparty;

      if(isPresent(nextPeerID, ignoredPeers)) continue;
      if(isPresent(*channelID, ignoredChannels)) continue;

      tmpDist = distance[bestPeerID].distance + channel->policy.timelock;

      channelInfo = hashTableGet(channelInfos, channel->channelInfoID);
      capacity = channelInfo->capacity;

      if(tmpDist < distance[nextPeerID].distance && amount<=capacity) {
        distance[nextPeerID].peer = nextPeerID;
        distance[nextPeerID].distance = tmpDist;

        previousPeer[nextPeerID].channel = *channelID;
        previousPeer[nextPeerID].peer = bestPeerID;

        heapInsert(distanceHeap, &distance[nextPeerID], compareDistance);
      }
      }

    }

  if(previousPeer[target].peer == -1) {
    printf ("no path available!\n");
    strcpy(error, "noPath");
    return NULL;
  }

  hops=arrayInitialize(HOPSLIMIT);
  prev=target;
  while(prev!=source) {
    //    printf("%ld ", previousPeer[prev].peer);
    hop = GC_MALLOC(sizeof(PathHop));
    hop->channel = previousPeer[prev].channel;
    hop->sender = previousPeer[prev].peer;
    channel = hashTableGet(channels, hop->channel);
    hop->receiver = channel->counterparty;
    hops=arrayInsert(hops, hop );
    prev = previousPeer[prev].peer;
  }

  if(arrayLen(hops)>HOPSLIMIT) {
    strcpy(error, "limitExceeded");
    return NULL;
  }

  arrayReverse(hops);

  return hops;
}

int isSamePath(Array*rootPath, Array*path) {
  long i;
  PathHop* hop1, *hop2;

  for(i=0; i<arrayLen(rootPath); i++) {
    hop1=arrayGet(rootPath, i);
    hop2=arrayGet(path, i);
    if(hop1->channel != hop2->channel)
      return 0;
  }

  return 1;
}

int comparePath(Array* path1, Array* path2){
  long len1, len2;
  len1 = arrayLen(path1);
  len2=arrayLen(path2);

  if(len1==len2) return 0;
  else if (len1<len2) return -1;
  else return 1;
}

Array* findPaths(long source, long target, double amount){
  Array* startingPath, *firstPath, *prevShortest, *rootPath, *path, *spurPath, *newPath, *nextShortestPath;
  Array* ignoredChannels, *ignoredPeers;
 PathHop *hop;
  long i, k, j, spurNode, newPathLen;
  Array* shortestPaths;
  Heap* candidatePaths;

  candidatePaths = heapInitialize(100);

  ignoredPeers=arrayInitialize(2);
  ignoredChannels=arrayInitialize(2);

  shortestPaths = arrayInitialize(100);

  startingPath=dijkstra(source, target, amount, ignoredPeers, ignoredChannels);
  if(startingPath==NULL) return NULL;

  firstPath = arrayInitialize(arrayLen(startingPath)+1);
  hop = GC_MALLOC(sizeof(PathHop));
  hop->receiver = source;
  firstPath=arrayInsert(firstPath, hop);
  for(i=0; i<arrayLen(startingPath); i++) {
    hop = arrayGet(startingPath, i);
    firstPath=arrayInsert(firstPath, hop);
  }

  shortestPaths = arrayInsert(shortestPaths, firstPath);

  for(k=1; k<100; k++) {
    prevShortest = arrayGet(shortestPaths, k-1);

    for(i=0; i<arrayLen(prevShortest)-1; i++) {
      hop = arrayGet(prevShortest, i);
      spurNode = hop->receiver;

      //roothPath = prevshortest[:i+1]
      rootPath = arrayInitialize(i+1);
      for(j=0; j<i+1; j++) {
        hop = arrayGet(prevShortest, j);
        rootPath=arrayInsert(rootPath, hop);
      }

      for(j=0; j<arrayLen(shortestPaths); j++) {
        path=arrayGet(shortestPaths, j);
        if(arrayLen(path)>i+1) {
          if(isSamePath(rootPath, path)) {
            hop = arrayGet(path, i+1);
            ignoredChannels = arrayInsert(ignoredChannels, &(hop->channel));
          }
        }
      }

      for(j=0; j<arrayLen(rootPath); j++) {
        hop = arrayGet(rootPath, j);
        if(hop->receiver == spurNode) continue;
        ignoredPeers = arrayInsert(ignoredPeers, &(hop->receiver));
      }

      spurPath = dijkstra(spurNode, target, amount, ignoredPeers, ignoredChannels);

      if(spurPath==NULL) {
        if(strcmp(error, "noPath")==0) continue;
        else return NULL;
      }

      newPathLen = arrayLen(rootPath) + arrayLen(spurPath);
      newPath = arrayInitialize(newPathLen);
      for(j=0; j<arrayLen(rootPath); j++) {
        hop = arrayGet(rootPath, j);
        newPath = arrayInsert(newPath, hop);
      }
      for(j=0; j<arrayLen(spurPath); j++) {
        hop = arrayGet(spurPath, j);
        newPath = arrayInsert(newPath, hop);
      }

      heapInsert(candidatePaths, newPath, comparePath);
    }

    if(heapLen(candidatePaths)==0) break;

    nextShortestPath = heapPop(candidatePaths, comparePath);
    shortestPaths = arrayInsert(shortestPaths, nextShortestPath);
  }

  return shortestPaths;

}

/*
Array* transformPathIntoRoute(Array* path, double amount) {


}*/
