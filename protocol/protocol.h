#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../utils/array.h"
#include "findRoute.h"
#include "../utils/hashTable.h"
#include "../simulator/event.h"

extern long channelIndex, peerIndex, channelInfoIndex, paymentIndex;
extern long nPeers;
extern long nChannels;
extern HashTable* peers;
extern HashTable* channels;
extern HashTable* channelInfos;
extern HashTable* payments;


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
  long otherChannelDirectionID;
  Policy policy;
  double balance;
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

void initializeProtocolData(long nPeers, long nChannels);

Peer* createPeer(long ID, long channelsSize);

ChannelInfo* createChannelInfo(long ID, long peer1, long peer2, double capacity);

Channel* createChannel(long ID, long channelInfoID, long counterparty, Policy policy, double balance);

Payment* createPayment(long ID, long sender, long receiver, double amount);

void connectPeers(long peer1, long peer2);

void findRoute(Event* event);

void sendPayment(Event* event);

void forwardPayment(Event* event);

void receivePayment(Event* event);

long getEdgeIndex(Peer*n);

#endif
