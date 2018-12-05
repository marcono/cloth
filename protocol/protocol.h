#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../utils/array.h"
#include "findRoute.h"
#include "../utils/hashTable.h"
#include "../simulator/event.h"
#include "gsl_rng.h"
#include "gsl_randist.h"
#include <stdint.h>
#include <pthread.h>

extern long channelIndex, peerIndex, channelInfoIndex, paymentIndex;
/* extern HashTable* peers; */
/* extern HashTable* channels; */
/* extern HashTable* channelInfos; */
extern double pUncoopBeforeLock, pUncoopAfterLock;
extern gsl_rng *r;
extern const gsl_rng_type * T;
extern gsl_ran_discrete_t* uncoop_before_discrete, *uncoop_after_discrete;
FILE *csvPeer, *csvChannel, *csvChannelInfo;
extern long nDijkstra;

typedef struct policy {
  uint32_t feeBase;
  uint32_t feeProportional;
  uint32_t minHTLC;
  uint16_t timelock;
} Policy;

typedef struct ignored{
  long ID;
  uint64_t time;
} Ignored;

typedef struct peer {
  long ID;
  int withholdsR;
  uint64_t initialFunds;
  uint64_t remainingFunds;
  Array* channel;
  Array* ignoredPeers;
  Array* ignoredChannels;
} Peer;

typedef struct channelInfo {
  long ID;
  long peer1;
  long peer2;
  long channelDirection1;
  long channelDirection2;
  uint64_t capacity;
  uint32_t latency;
  unsigned int isClosed;
} ChannelInfo;

typedef struct channel {
  long ID;
  long channelInfoID;
  long counterparty;
  long otherChannelDirectionID;
  Policy policy;
  uint64_t balance;
  unsigned int isClosed;
} Channel;

typedef struct payment{
  long ID;
  long sender;
  long receiver;
  uint64_t amount; //millisatoshis
  Route* route;
  int isSuccess;
  int uncoopAfter;
  int uncoopBefore;
  int isTimeout;
  uint64_t startTime;
  uint64_t endTime;
  int attempts;
} Payment;

extern Array* peers;
extern Array* channels;
extern Array* channelInfos;

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

void initializeProtocolData(long nPeers, long nChannels, double pUncoopBefore, double pUncoopAfter, double RWithholding, int gini, int sigma, long capacityPerChannel, unsigned int isPreproc);

Peer* createPeer(long ID, long channelsSize);

Peer* createPeerPostProc(long ID, int withholdsR);

ChannelInfo* createChannelInfo(long ID, long peer1, long peer2, uint32_t latency);

ChannelInfo* createChannelInfoPostProc(long ID, long direction1, long direction2, long peer1, long peer2, uint64_t capacity, uint32_t latency);

Channel* createChannel(long ID, long channelInfoID, long counterparty, Policy policy);

Channel* createChannelPostProc(long ID, long channelInfoID, long otherDirection, long counterparty, uint64_t balance, Policy policy);

Payment* createPayment(long ID, long sender, long receiver, uint64_t amount);

void connectPeers(long peer1, long peer2);

void createTopologyFromCsv(unsigned int isPreproc);

//TODO: sposta queste due funzioni in qualche file utils.h
int isPresent(long element, Array* longArray);

int isEqual(long *a, long *b);

uint64_t computeFee(uint64_t amountToForward, Policy policy);

void findRoute(Event* event);

void sendPayment(Event* event);

void forwardPayment(Event* event);

void receivePayment(Event* event);

void forwardSuccess(Event* event);

void receiveSuccess(Event* event);

void forwardFail(Event* event);

void receiveFail(Event* event);

void* dijkstraThread(void*arg);

long getEdgeIndex(Peer*n);

#endif
