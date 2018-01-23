#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../utils/array.h"
#include "findRoute.h"
#include "../utils/hashTable.h"
#include "../simulator/event.h"
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <stdint.h>

extern long channelIndex, peerIndex, channelInfoIndex, paymentIndex;
extern HashTable* peers;
extern HashTable* channels;
extern HashTable* channelInfos;
extern HashTable* payments;
extern double pUncoopBeforeLock, pUncoopAfterLock;
extern gsl_rng *r;
extern const gsl_rng_type * T;
extern gsl_ran_discrete_t* uncoop_before_discrete, *uncoop_after_discrete;


typedef struct policy {
  uint32_t feeBase;
  uint32_t feeProportional;
  int timelock;
} Policy;

typedef struct peer {
  long ID;
  int withholdsR;
  uint64_t initialFunds;
  uint64_t remainingFunds;
  Array* channel;
} Peer;

typedef struct channelInfo {
  long ID;
  long peer1;
  long peer2;
  long channelDirection1;
  long channelDirection2;
  uint64_t capacity;
  uint32_t latency;
} ChannelInfo;

typedef struct channel {
  long ID;
  long channelInfoID;
  long counterparty;
  long otherChannelDirectionID;
  Policy policy;
  uint64_t balance;
} Channel;

typedef struct payment{
  long ID;
  long sender;
  long receiver;
  uint64_t amount; //millisatoshis
  Route* route;
  Array* ignoredPeers;
  Array* ignoredChannels;
  int isSuccess;
  int isAPeerUncoop;
  uint64_t startTime;
  uint64_t endTime;
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

void initializeProtocolData(long nPeers, long nChannels, double pUncoopBefore, double pUncoopAfter, double RWithholding, double gini);

Peer* createPeer(long ID, long channelsSize);

ChannelInfo* createChannelInfo(long ID, long peer1, long peer2, uint32_t latency);

Channel* createChannel(long ID, long channelInfoID, long counterparty, Policy policy);

Payment* createPayment(long ID, long sender, long receiver, uint64_t amount);

void connectPeers(long peer1, long peer2);

int isPresent(long element, Array* longArray);

uint64_t computeFee(uint64_t amountToForward, Policy policy);

void findRoute(Event* event);

void sendPayment(Event* event);

void forwardPayment(Event* event);

void receivePayment(Event* event);

void forwardSuccess(Event* event);

void receiveSuccess(Event* event);

void forwardFail(Event* event);

void receiveFail(Event* event);

long getEdgeIndex(Peer*n);

#endif
