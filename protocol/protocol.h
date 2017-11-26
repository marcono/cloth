#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../utils/array.h"

long channelID=0, peerID=0, channelInfoID=0;

typedef struct policy {
  double fee;
  double timelock;
} Policy;

typedef struct peer {
  long ID;
  long channelsSize;
  Array* channel;
} Peer;

typedef struct channelInfo {
  long ID;
  long peer1;
  long peer2;
  double capacity;
} ChannelInfo;

typedef struct channel {
  long ID;
  long channelInfoID;
  long counterparty;
  Policy policy;
} Channel;

/*
typedef struct peer {
  long ID;
  Array* node;
  Array* channel;
} Peer;

typedef struct channel{
  long ID;
  long counterparty;
  double capacity;
  double balance;
  Policy policy;
} Channel;
*/

Peer* createPeer(long channelsSize);

ChannelInfo* createChannelInfo(long peer1, long peer2, double capacity);

Channel* createChannel(long channelInfoID, long counterparty, Policy policy);

long getEdgeIndex(Peer*n);

#endif
