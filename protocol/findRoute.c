#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../gc-7.2/include/gc.h"
#include "../simulator/initialize.h"
#include "../protocol/protocol.h"
#include "../utils/heap.h"
#include "../utils/array.h"
#include "findRoute.h"
#define INF UINT16_MAX
#define HOPSLIMIT 20

//FIXME: non globale ma passato per riferimento a dijkstra
char error[100];
uint32_t** dist;
PathHop** next;

void initializeFindRoute() {
  distanceHeap=NULL;
  previousPeer = NULL;
  distance = NULL;
}

int compareDistance(Distance* a, Distance* b) {
  uint16_t d1, d2;
  d1=a->distance;
  d2=b->distance;
  if(d1==d2)
    return 0;
  else if(d1<d2)
    return -1;
  else
    return 1;
}

void floydWarshall() {
  long i, j, k;
  ChannelInfo* channelInfo;
  Channel* direction1, *direction2;

  dist = GC_MALLOC(sizeof(uint32_t*)*peerIndex);
  next = GC_MALLOC(sizeof(PathHop*)*peerIndex);
  //  paths = GC_MALLOC(sizeof(Array**)*peerIndex);
  for(i=0; i<peerIndex; i++) {
    dist[i] = GC_MALLOC(sizeof(uint32_t)*peerIndex);
    next[i] = GC_MALLOC(sizeof(PathHop)*peerIndex);
    //paths[i] = GC_MALLOC(sizeof(Array*)*peerIndex);
  }


  for(i=0; i<peerIndex; i++){
    for(j=0; j<peerIndex; j++) {
      if(i==j)
        dist[i][j] = 0;
      else
        dist[i][j] = INF;

      next[i][j].channel = -1;
      //      paths[i][j] = arrayInitialize(10);
    }
  }

  for(i=0; i<channelInfoIndex; i++) {
    channelInfo = hashTableGet(channelInfos, i);
    direction1 = hashTableGet(channels, channelInfo->channelDirection1);
    direction2 = hashTableGet(channels, channelInfo->channelDirection2);
    dist[channelInfo->peer1][channelInfo->peer2] = direction1->policy.timelock;
    dist[channelInfo->peer2][channelInfo->peer1] = direction2->policy.timelock;
    next[channelInfo->peer1][channelInfo->peer2].sender = channelInfo->peer1;
    next[channelInfo->peer1][channelInfo->peer2].receiver = channelInfo->peer2;
    next[channelInfo->peer1][channelInfo->peer2].channel = channelInfo->channelDirection1;
    next[channelInfo->peer2][channelInfo->peer1].sender = channelInfo->peer2;
    next[channelInfo->peer2][channelInfo->peer1].receiver = channelInfo->peer1;
    next[channelInfo->peer2][channelInfo->peer1].channel = channelInfo->channelDirection2;
  }

  for(k=0; k<peerIndex; k++) {
    for(i=0; i<peerIndex; i++){
      for(j=0; j<peerIndex; j++){
        if(dist[i][j] > dist[i][k] + dist[k][j]) {
          dist[i][j] = dist[i][k] + dist[k][j];
          next[i][j] = next[i][k];
        }
      }
    }
  }

  /*
  for(i=0; i<paymentIndex; i++) {
    payment = hashTableGet(payments, i);
    source = payment->sender;
    destination = payment->receiver;

    if(next[source][destination].channel==-1)
      continue;

    paths[source][destination] = arrayInsert(paths[source][destination], &(next[source][destination]));
    nextPeer = next[source][destination].receiver;
    while(nextPeer != destination ) {
      paths[source][destination] = arrayInsert(paths[source][destination], &(next[nextPeer][destination]));
      nextPeer = next[nextPeer][destination].receiver;
    }

    if(arrayLen(paths[source][destination])>HOPSLIMIT)
      arrayDeleteAll(paths[source][destination]);
  }
  */

}

Array* getPath(long source, long destination) {
  Array* path;
  long nextPeer;

  if(next[source][destination].channel==-1) {
    return NULL;
  }
  path = arrayInitialize(10);

  path = arrayInsert(path, &(next[source][destination]));
  nextPeer = next[source][destination].receiver;
  while(nextPeer != destination ) {
    path = arrayInsert(path, &(next[nextPeer][destination]));
    nextPeer = next[nextPeer][destination].receiver;
  }

  if(arrayLen(path)>HOPSLIMIT)
    return NULL;

  return path;
}

Array* dijkstra(long source, long target, uint64_t amount, Array* ignoredPeers, Array* ignoredChannels) {
  Distance *d;
  long i, bestPeerID, j,*channelID, nextPeerID, prev;
  //  Heap *distanceHeap;
  Peer* bestPeer;
  Channel* channel;
  ChannelInfo* channelInfo;
  uint32_t tmpDist;
  uint64_t capacity;
  //DijkstraHop *previousPeer;
  Array* hops;
  PathHop* hop;

  printf("DIJKSTRA\n");

  if(distance==NULL) {
    distance = GC_MALLOC(sizeof(Distance)*peerIndex);
  }

  if(previousPeer==NULL) {
    previousPeer = GC_MALLOC(sizeof(DijkstraHop)*peerIndex);
  }

  if(distanceHeap==NULL) {
    distanceHeap = heapInitialize(peerIndex);
  }

  for(i=0; i<peerIndex; i++){
    distance[i].peer = i;
    distance[i].distance = INF;
    previousPeer[i].channel = -1;
    previousPeer[i].peer = -1;
  }

  distance[source].peer = source;
  distance[source].distance = 0;

  //TODO: e' safe passare l'inidrizzo dell'i-esimo elemento dell'array?
  distanceHeap =  heapInsert(distanceHeap, &distance[source], compareDistance); 

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

        distanceHeap = heapInsert(distanceHeap, &distance[nextPeerID], compareDistance);
      }
      }

    }

  if(previousPeer[target].peer == -1) {
    //    printf ("no path available!\n");
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

  //  GC_FREE(previousPeer);
  //GC_FREE(distance);
  //heapFree(distanceHeap);

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


Route* routeInitialize(long nHops) {
  Route* r;

  r = GC_MALLOC(sizeof(Route));
  r->routeHops = arrayInitialize(nHops);
  r->totalAmount = 0.0;
  r->totalFee = 0.0;
  r->totalTimelock = 0;

  return r;
}

//TODO: sposta computeFee nei file protocol

Route* transformPathIntoRoute(Array* pathHops, uint64_t amountToSend, int finalTimelock) {
  PathHop *pathHop;
  RouteHop *routeHop, *nextRouteHop;
  Route *route;
  long nHops, i;
  uint64_t fee, currentChannelCapacity;
  Channel* channel;
  Policy currentChannelPolicy, nextChannelPolicy;
  ChannelInfo* channelInfo;

  nHops = arrayLen(pathHops);
  route = routeInitialize(nHops);

  for(i=nHops-1; i>=0; i--) {
    pathHop = arrayGet(pathHops, i);

    channel = hashTableGet(channels, pathHop->channel);
    currentChannelPolicy = channel->policy;
    channelInfo = hashTableGet(channelInfos,channel->channelInfoID);
    currentChannelCapacity = channelInfo->capacity;

    routeHop = GC_MALLOC(sizeof(RouteHop));
    routeHop->pathHop = pathHop;

    if(i == arrayLen(pathHops)-1) {
      routeHop->amountToForward = amountToSend;
      route->totalAmount += amountToSend;

      if(nHops == 1) {
        routeHop->timelock = 0;
        route->totalTimelock = 0;
      }
      else {
        routeHop->timelock = currentChannelPolicy.timelock;
        route->totalTimelock += currentChannelPolicy.timelock;
      }
    }
    else {
      fee = computeFee(nextRouteHop->amountToForward, nextChannelPolicy);
      routeHop->amountToForward = nextRouteHop->amountToForward + fee;
      route->totalFee += fee;
      route->totalAmount += fee;

      routeHop->timelock = nextRouteHop->timelock + currentChannelPolicy.timelock;
      route->totalTimelock += currentChannelPolicy.timelock;
    }

    //TODO: mettere stringa con messaggio di errore tra i parametri della funzione
    if(routeHop->amountToForward > currentChannelCapacity)
      return NULL;

    route->routeHops = arrayInsert(route->routeHops, routeHop);
    nextChannelPolicy = currentChannelPolicy;
    nextRouteHop = routeHop;

    }

  arrayReverse(route->routeHops);

  return route;
  }


void printHop(RouteHop* hop){
  printf("Sender %ld, Receiver %ld, Channel %ld\n", hop->pathHop->sender, hop->pathHop->receiver, hop->pathHop->channel);
}

