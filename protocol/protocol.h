#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../utils/array.h"
#include "findRoute.h"

extern long channelID, peerID, channelInfoID, paymentID;

typedef struct policy {
  double feeBase;
  double feeProportional;
  int timelock;
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

typedef struct payment{
  long ID;
  long sender;
  long receiver;
  double amount;
  Route* route;
} Payment;

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

Peer* createPeer(long ID, long channelsSize);

ChannelInfo* createChannelInfo(long ID, long peer1, long peer2, double capacity);

Channel* createChannel(long ID, long channelInfoID, long counterparty, Policy policy);

Payment* createPayment(long ID, long sender, long receiver, double amount);

void connectPeers(long peer1, long peer2);

long getEdgeIndex(Peer*n);

#endif
