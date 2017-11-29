#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../utils/array.h"

extern long channelID, peerID, channelInfoID;

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

void initializeProtocol();

Peer* createPeer(long channelsSize);

ChannelInfo* createChannelInfo(long peer1, long peer2, double capacity);

Channel* createChannel(long channelInfoID, long counterparty, Policy policy);

void connectPeers(long peer1, long peer2);

long getEdgeIndex(Peer*n);

#endif
