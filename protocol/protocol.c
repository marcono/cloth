#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_math.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>

#include "../gc-7.2/include/gc.h"
#include "protocol.h"
#include "../utils/array.h"
#include "../utils/hashTable.h"
#include "../utils/heap.h"
#include "../simulator/initialize.h"
#include "findRoute.h"
#include "../simulator/event.h"
#include "../simulator/stats.h"
#include "../global.h"

#define MAXMSATOSHI 5E17 //5 millions  bitcoin
#define MAXTIMELOCK 100
#define MINTIMELOCK 10
#define MAXFEEBASE 1000
#define MINFEEBASE 100
#define MAXFEEPROP 10
#define MINFEEPROP 1
#define MAXLATENCY 10
#define MINLATENCY 1
#define MINBALANCE 1E2
#define MAXBALANCE 1E11
#define FAULTYLATENCY 60000 //1 minute waiting for a peer not responding
#define INF UINT16_MAX
#define HOPSLIMIT 20

//TODO: creare ID randomici e connettere peer "vicini" usando il concetto
// di vicinanza di chord; memorizzare gli id dei peer in un Array di long
// per avere facile accesso a tutti i peer nel momento della creazione dei canali
// (per controllare invece se un certo ID e' gia' stato usato, usare direttamente l'hashtable dei peer
// perche' e' piu' efficiente)
// CONTROLLARE SE QUESTO POTREBBE ESSERE UN PROBLEMA PER GLI ALGORITMI DI ROUTING

long channelIndex, peerIndex, channelInfoIndex, paymentIndex;
/* HashTable* peers; */
/* HashTable* channels; */
/* HashTable* channelInfos; */
gsl_rng *r;
const gsl_rng_type * T;
gsl_ran_discrete_t* uncoop_before_discrete, *uncoop_after_discrete;

Array* peers;
Array* channels;
Array* channelInfos;

long nDijkstra;

Peer* createPeer(long ID, long channelsSize) {
  Peer* peer;

  peer = malloc(sizeof(Peer));
  peer->ID=ID;
  peer->channel = arrayInitialize(channelsSize);
  peer->initialFunds = 0;
  peer->remainingFunds = 0;
  peer->withholdsR = 0;

  peerIndex++;

  return peer;
}

ChannelInfo* createChannelInfo(long ID, long peer1, long peer2, uint32_t latency) {
  ChannelInfo* channelInfo;

  channelInfo = malloc(sizeof(ChannelInfo));
  channelInfo->ID = ID;
  channelInfo->peer1 = peer1;
  channelInfo->peer2 = peer2;
  channelInfo->latency = latency;
  channelInfo->capacity = 0;
  channelInfo->isClosed = 0;

  channelInfoIndex++;

  return channelInfo;
}


Channel* createChannel(long ID, long channelInfoID, long counterparty, Policy policy){
  Channel* channel;

  channel = malloc(sizeof(Channel));
  channel->ID = ID;
  channel->channelInfoID = channelInfoID;
  channel->counterparty = counterparty;
  channel->policy = policy;
  channel->balance = 0;
  channel->isClosed = 0;

  channelIndex++;

  return channel;
}

Payment* createPayment(long ID, long sender, long receiver, uint64_t amount) {
  Payment * p;

  p = malloc(sizeof(Payment));
  p->ID=ID;
  p->sender= sender;
  p->receiver = receiver;
  p->amount = amount;
  p->route = NULL;
  p->ignoredChannels = arrayInitialize(10);
  p->ignoredPeers = arrayInitialize(10);
  p->isSuccess = 0;
  p->isAPeerUncoop = 0;
  p->startTime = 0;
  p->endTime = 0;
  p->attempts = -1;

  paymentIndex++;

  return p;
}

Peer* createPeerPostProc(long ID, int withholdsR) {
  Peer* peer;

  peer = malloc(sizeof(Peer));
  peer->ID=ID;
  peer->channel = arrayInitialize(5);
  peer->initialFunds = 0;
  peer->remainingFunds = 0;
  peer->withholdsR = withholdsR;

  peerIndex++;

  return peer;
}

ChannelInfo* createChannelInfoPostProc(long ID, long direction1, long direction2, long peer1, long peer2, uint64_t capacity, uint32_t latency) {
  ChannelInfo* channelInfo;

  channelInfo = malloc(sizeof(ChannelInfo));
  channelInfo->ID = ID;
  channelInfo->channelDirection1 = direction1;
  channelInfo->channelDirection2 = direction2;
  channelInfo->peer1 = peer1;
  channelInfo->peer2 = peer2;
  channelInfo->latency = latency;
  channelInfo->capacity = capacity;
  channelInfo->isClosed = 0;

  channelInfoIndex++;

  return channelInfo;
}

Channel* createChannelPostProc(long ID, long channelInfoID, long otherDirection, long counterparty, uint64_t balance, Policy policy){
  Channel* channel;

  channel = malloc(sizeof(Channel));
  channel->ID = ID;
  channel->channelInfoID = channelInfoID;
  channel->counterparty = counterparty;
  channel->otherChannelDirectionID = otherDirection;
  channel->policy = policy;
  channel->balance = balance;
  channel->isClosed = 0;

  channelIndex++;

  return channel;
}




void connectPeers(long peerID1, long peerID2) {
  Peer* peer1, *peer2;
  Policy policy1, policy2;
  Channel* firstChannelDirection, *secondChannelDirection; //TODO: rinominare channelInfo->channel e channel->channelDirection
  ChannelInfo *channelInfo;
  uint32_t latency;
  uint64_t balance; 

  peer1 = arrayGet(peers, peerID1);
  peer2 = arrayGet(peers, peerID2);

  latency = gsl_rng_uniform_int(r, MAXLATENCY - MINLATENCY) + MINLATENCY;
  channelInfo = createChannelInfo(channelInfoIndex, peer1->ID, peer2->ID, latency);
  //  hashTablePut(channelInfos, channelInfo->ID, channelInfo);
  channelInfos = arrayInsert(channelInfos, channelInfo);

  balance = gsl_rng_uniform_int(r, 10) + 1;
  //  exponent = gsl_ran_poisson(r, 4.5);
  balance = balance*gsl_pow_uint(10, gsl_rng_uniform_int(r, 7)+4); //balance*10^exponent, where exponent is a uniform number in [4,11]

  policy1.feeBase = gsl_rng_uniform_int(r, MAXFEEBASE - MINFEEBASE) + MINFEEBASE;
  policy1.feeProportional = (gsl_rng_uniform_int(r, MAXFEEPROP-MINFEEPROP)+MINFEEPROP)*1000;
  policy1.timelock = gsl_rng_uniform_int(r, MAXTIMELOCK-MINTIMELOCK)+MINTIMELOCK;
  firstChannelDirection = createChannel(channelIndex, channelInfo->ID, peer2->ID, policy1);
  firstChannelDirection->balance = balance;
  //  hashTablePut(channels, firstChannelDirection->ID, firstChannelDirection);
  channels = arrayInsert(channels, firstChannelDirection);
  peer1->channel = arrayInsert(peer1->channel, &(firstChannelDirection->ID));
  channelInfo->channelDirection1 = firstChannelDirection->ID;

  policy2.feeBase = gsl_rng_uniform_int(r, MAXFEEBASE - MINFEEBASE) + MINFEEBASE;
  policy2.feeProportional = (gsl_rng_uniform_int(r, MAXFEEPROP-MINFEEPROP)+MINFEEPROP)*1000;
  policy2.timelock = gsl_rng_uniform_int(r, MAXTIMELOCK-MINTIMELOCK)+MINTIMELOCK;
  secondChannelDirection = createChannel(channelIndex, channelInfo->ID, peer1->ID, policy2);
  secondChannelDirection->balance = balance;
  //hashTablePut(channels,secondChannelDirection->ID, secondChannelDirection);
  channels = arrayInsert(channels, secondChannelDirection);
  peer2->channel =arrayInsert(peer2->channel, &(secondChannelDirection->ID));
  channelInfo->channelDirection2 = secondChannelDirection->ID;

  firstChannelDirection->otherChannelDirectionID = secondChannelDirection->ID;
  secondChannelDirection->otherChannelDirectionID = firstChannelDirection->ID;

  channelInfo->capacity = firstChannelDirection->balance + secondChannelDirection->balance;

}

void computePeersInitialFunds(double gini) {
  long i;
  Peer* peer;

  for(i=0; i<peerIndex; i++) {
    peer = arrayGet(peers, i);
    peer->initialFunds = MAXMSATOSHI/peerIndex;
    peer->remainingFunds = peer->initialFunds;
  }
}

/*
  uint64_t computeChannelBalance(Peer *peer) {
  uint64_t channelBalance;

  channelBalance = peer->initialFunds/4; //TODO: al posto di 4, il numero di canali per peer o altro
  if(peer->remainingFunds < channelBalance) {
  channelBalance = peer->remainingFunds;
  peer->remainingFunds=0.0;
  }
  else
  peer->remainingFunds -= channelBalance;

  return channelBalance;
  }
*/


double computeGini() {
  long i, j;
  __uint128_t num=0, den=0;
  __uint128_t difference;
  ChannelInfo *channeli, *channelj;
  double gini;

  for(i=0;i<channelInfoIndex; i++) {
    channeli = arrayGet(channelInfos, i);
    den += channeli->capacity;
    for(j=0; j<channelInfoIndex; j++){
      if(i==j) continue;
      channelj = arrayGet(channelInfos, j);
      if(channeli->capacity > channelj->capacity)
        difference = channeli->capacity - channelj->capacity;
      else
        difference = channelj->capacity - channeli->capacity;
      num += difference;
    }
  }

  den = 2*channelInfoIndex*den;

  gini = num/(den*1.0);

  return gini;
}

void initializeTopologyPreproc(long nPeers, long nChannels, double RWithholding, int gini) {
  long i, j, peerIDIndex, channelInfoIDIndex, channelIDIndex, peer1, peer2, direction1, direction2, info;
  double RwithholdingP[] = {1-RWithholding, RWithholding}, coeff1, coeff2;
  gsl_ran_discrete_t* RwithholdingDiscrete;
  int *peerChannels;
  uint32_t latency;
  uint64_t balance, capacity;
  Policy policy1, policy2;
  long thresh1, thresh2, counter=0;
  uint64_t funds1, funds2, funds3;

  switch(gini) {
  case 1:
    coeff1 = 0.3;
    coeff2 = 0.3;
    break;
  case 2:
    coeff1 = 0.15;
    coeff2 = 0.2;
    break;
  case 3:
    coeff1 = 0.1;
    coeff2 = 0.05;
    break;
  case 4:
    coeff1 = 0.01;
    coeff2 = 0.05;
    break;
  case 5:
    coeff1 = 1.0/(nPeers*nChannels);
    coeff2 = 1;
    break;
  default:
    printf("ERROR wrong preinput gini level\n");
    return;
  }

  thresh1 = nPeers*nChannels*coeff1;
  thresh2 = nPeers*nChannels*coeff2;

  if(gini != 5) {
  funds1 = (MAXMSATOSHI/3)/thresh1;
  funds2 = (MAXMSATOSHI/3)/thresh2;
  funds3 = (MAXMSATOSHI/3)/(nPeers*nChannels - (thresh1 + thresh2));
  }
  else {
  funds1 = MAXMSATOSHI;
  funds2 = 1;
  funds3 = 1;
  }

  csvPeer = fopen("peer.csv", "w");
  if(csvPeer==NULL) {
    printf("ERROR cannot open file peer.csv\n");
    return;
  }
  fprintf(csvPeer, "ID,WithholdsR\n");

  csvChannelInfo = fopen("channelInfo.csv", "w");
  if(csvChannelInfo==NULL) {
    printf("ERROR cannot open file channelInfo.csv\n");
    return;
  }
  fprintf(csvChannelInfo, "ID,Direction1,Direction2,Peer1,Peer2,Capacity,Latency\n");

  csvChannel = fopen("channel.csv", "w");
  if(csvChannel==NULL) {
    printf("ERROR cannot open file channel.csv\n");
    return;
  }
  fprintf(csvChannel, "ID,ChannelInfo,OtherDirection,Counterparty,Balance,FeeBase,FeeProportional,Timelock\n");


  RwithholdingDiscrete = gsl_ran_discrete_preproc(2, RwithholdingP);

  peerIDIndex=0;
  for(i=0; i<nPeers; i++){
    fprintf(csvPeer, "%ld,%ld\n", peerIDIndex, gsl_ran_discrete(r, RwithholdingDiscrete));
    peerIDIndex++;
  }


  /*
  rewind(csvPeer);
  unsigned long nR=0, withholdsR=0, ID=0;
  char row[100];
  while(fgets(row, 100, csvPeer)!=NULL) {
    sscanf(row, "%ld,%ld\n", &ID, &withholdsR);
    if(withholdsR) ++nR;
  }
  */

  peerChannels = malloc(peerIDIndex*sizeof(int));
  for(i=0;i<peerIDIndex;i++) 
    peerChannels[i] = 0;

  channelInfoIDIndex = channelIDIndex= 0;
  for(i=0; i<peerIDIndex; i++) {
    peer1 = i;
    for(j=0; j<nChannels && (peerChannels[peer1] < nChannels); j++){

      do {
        peer2 = gsl_rng_uniform_int(r,peerIDIndex);
      }while(peer2==peer1);

      if(peerChannels[peer2]>=nChannels) continue;

      ++peerChannels[peer1];
      ++peerChannels[peer2];
      info = channelInfoIDIndex;
      ++channelInfoIDIndex;
      direction1 = channelIDIndex;
      ++channelIDIndex;
      direction2 = channelIDIndex;
      ++channelIDIndex;

      latency = gsl_rng_uniform_int(r, MAXLATENCY - MINLATENCY) + MINLATENCY;
      /* balance = gsl_rng_uniform_int(r, 10) + 1; */
      /* balance = balance*gsl_pow_uint(10, gsl_rng_uniform_int(r, 7)+4); */
      /* capacity = balance*2; */

      counter++;
      if(counter <= thresh1)
        capacity = funds1;
      else if (counter > thresh1 && counter <= thresh1 + thresh2)
        capacity = funds2;
      else
        capacity = funds3;

      balance = capacity/2;

      policy1.feeBase = gsl_rng_uniform_int(r, MAXFEEBASE - MINFEEBASE) + MINFEEBASE;
      policy1.feeProportional = (gsl_rng_uniform_int(r, MAXFEEPROP-MINFEEPROP)+MINFEEPROP)*1000;
      policy1.timelock = gsl_rng_uniform_int(r, MAXTIMELOCK-MINTIMELOCK)+MINTIMELOCK;
      policy2.feeBase = gsl_rng_uniform_int(r, MAXFEEBASE - MINFEEBASE) + MINFEEBASE;
      policy2.feeProportional = (gsl_rng_uniform_int(r, MAXFEEPROP-MINFEEPROP)+MINFEEPROP)*1000;
      policy2.timelock = gsl_rng_uniform_int(r, MAXTIMELOCK-MINTIMELOCK)+MINTIMELOCK;

      fprintf(csvChannelInfo, "%ld,%ld,%ld,%ld,%ld,%ld,%d\n", info, direction1, direction2, peer1, peer2, capacity, latency);

      fprintf(csvChannel, "%ld,%ld,%ld,%ld,%ld,%d,%d,%d\n", direction1, info, direction2, peer2, balance, policy1.feeBase, policy1.feeProportional, policy1.timelock);

      fprintf(csvChannel, "%ld,%ld,%ld,%ld,%ld,%d,%d,%d\n", direction2, info, direction1, peer1, balance, policy2.feeBase, policy2.feeProportional, policy2.timelock);

    }

  }


  fclose(csvPeer);
  fclose(csvChannelInfo);
  fclose(csvChannel);

  printf("Change topology and press enter\n");
  scanf("%*c%*c");

}


void createTopologyFromCsv(unsigned int isPreproc) {
  char row[256], channelFile[64], infoFile[64];
  Peer* peer, *peer1, *peer2;
  long ID, direction1, direction2, peerID1, peerID2, channelInfoID, otherDirection, counterparty;
  Policy policy;
  int withholdsR;
  uint32_t latency;
  uint64_t capacity, balance;
  ChannelInfo* channelInfo;
  Channel* channel;

  if(isPreproc) {
    strcpy(channelFile, "channel.csv");
    strcpy(infoFile, "channelInfo.csv");
  }
  else {
    strcpy(channelFile, "channel_output.csv");
    strcpy(infoFile, "channelInfo_output.csv");
    }


  csvPeer = fopen("peer.csv", "r");
  if(csvPeer==NULL) {
    printf("ERROR cannot open file peer.csv\n");
    return;
  }

  fgets(row, 256, csvPeer);
  while(fgets(row, 256, csvPeer)!=NULL) {
    sscanf(row, "%ld,%d", &ID, &withholdsR);
    peer = createPeerPostProc(ID, withholdsR);
    //hashTablePut(peers,peer->ID, peer);
    peers = arrayInsert(peers, peer);
  }

  fclose(csvPeer);

  csvChannelInfo = fopen(infoFile, "r");
  if(csvChannelInfo==NULL) {
    printf("ERROR cannot open file %s\n", infoFile);
    return;
  }

  fgets(row, 256, csvChannelInfo);
  while(fgets(row, 256, csvChannelInfo)!=NULL) {
    sscanf(row, "%ld,%ld,%ld,%ld,%ld,%ld,%d", &ID, &direction1, &direction2, &peerID1, &peerID2, &capacity, &latency);
    channelInfo = createChannelInfoPostProc(ID, direction1, direction2, peerID1, peerID2, capacity, latency);
    //    hashTablePut(channelInfos, channelInfo->ID, channelInfo);
    channelInfos = arrayInsert(channelInfos, channelInfo);
    peer1 = arrayGet(peers, peerID1);
    peer1->channel = arrayInsert(peer1->channel, &(channelInfo->channelDirection1));
    peer2 = arrayGet(peers, peerID2);
    peer2->channel = arrayInsert(peer2->channel, &(channelInfo->channelDirection2));
  }

  fclose(csvChannelInfo);

  csvChannel = fopen(channelFile, "r");
  if(csvChannel==NULL) {
    printf("ERROR cannot open file %s\n", channelFile);
    return;
  }

  fgets(row, 256, csvChannel);
  while(fgets(row, 256, csvChannel)!=NULL) {
    sscanf(row, "%ld,%ld,%ld,%ld,%ld,%d,%d,%d", &ID, &channelInfoID, &otherDirection, &counterparty, &balance, &policy.feeBase, &policy.feeProportional, &policy.timelock);
    channel = createChannelPostProc(ID, channelInfoID, otherDirection, counterparty, balance, policy);
    //hashTablePut(channels, channel->ID, channel);
    channels = arrayInsert(channels, channel);
  }

  fclose(csvChannel);

}


void initializeTopology(long nPeers, long nChannels, double RWithholding, double gini) {
  long i, j, RWithholdingPeerID, nRWithholdingPeers, counterpartyID;
  Peer* peer, *counterparty;

  printf("inittopology\n");

  for(i=0; i<nPeers; i++){
    peer = createPeer(peerIndex, nChannels);
    //hashTablePut(peers, peer->ID, peer);
    peers = arrayInsert(peers, peer);
  }


  //  computePeersInitialFunds(gini);



  nRWithholdingPeers = nPeers*RWithholding;
  for(i=0; i < nRWithholdingPeers ;i++) {
    RWithholdingPeerID = gsl_rng_uniform_int(r,peerIndex);
    peer = arrayGet(peers, RWithholdingPeerID);
    peer->withholdsR = 1;
  }


  for(i=0; i<peerIndex; i++) {
    peer = arrayGet(peers, i);
    for(j=0; j<nChannels && (arrayLen(peer->channel) < nChannels); j++){

      do {
        counterpartyID = gsl_rng_uniform_int(r,peerIndex);
      }while(counterpartyID==peer->ID);

      counterparty = arrayGet(peers, counterpartyID);
      if(arrayLen(counterparty->channel)>=nChannels) continue;

      connectPeers(peer->ID, counterparty->ID);
    }



  }


}

void initializeProtocolData(long nPeers, long nChannels, double pUncoopBefore, double pUncoopAfter, double RWithholding, int gini, unsigned int isPreproc) {
  double beforeP[] = {pUncoopBefore, 1-pUncoopBefore};
  double afterP[] = {pUncoopAfter, 1-pUncoopAfter};

  //  initializeFindRoute();

  gsl_rng_env_setup();
  T = gsl_rng_default;
  r = gsl_rng_alloc (T);

  uncoop_before_discrete = gsl_ran_discrete_preproc(2, beforeP);
  uncoop_after_discrete = gsl_ran_discrete_preproc(2, afterP);

  channelIndex = peerIndex = channelInfoIndex = paymentIndex = 0;

  nDijkstra = 0;
  /* peers = hashTableInitialize(nPeers); */
  /* channels = hashTableInitialize(nChannels*nPeers*2); */
  /* channelInfos= hashTableInitialize(nChannels*nPeers); */

  peers = arrayInitialize(nPeers);
  channels = arrayInitialize(nChannels*nPeers);
  channelInfos = arrayInitialize(nChannels*nPeers);


  if(isPreproc)
    initializeTopologyPreproc(nPeers, nChannels, RWithholding, gini);

  createTopologyFromCsv(isPreproc);
  //initializeTopology(nPeers, nChannels, RWithholding, gini);

  printf("GINI: %lf\n", computeGini());

}


//TODO: farne unica versione, con element void, e mettere in array.c
int isPresent(long element, Array* longArray) {
  long i, *curr;

  if(longArray==NULL) return 0;

  for(i=0; i<arrayLen(longArray); i++) {
    curr = arrayGet(longArray, i);
    if(*curr==element) return 1;
  }

  return 0;
}

int isEqual(long* a, long* b) {
  return *a==*b;
}

void closeChannel(long channelInfoID) {
  long i;
  Peer *peer;
  ChannelInfo *channelInfo;
  Channel* direction1, *direction2;

  channelInfo = arrayGet(channelInfos, channelInfoID);
  direction1 = arrayGet(channels, channelInfo->channelDirection1);
  direction2 = arrayGet(channels, channelInfo->channelDirection2);

  channelInfo->isClosed = 1;
  direction1->isClosed = 1;
  direction2->isClosed = 1;

  printf("ChannelInfo %ld, ChannelDirection1 %ld, ChannelDirection2 %ld are now closed\n", channelInfo->ID, channelInfo->channelDirection1, channelInfo->channelDirection2);

  for(i = 0; i < peerIndex; i++) {
    peer = arrayGet(peers, i);
    arrayDelete(peer->channel, &(channelInfo->channelDirection1), isEqual);
    arrayDelete(peer->channel, &(channelInfo->channelDirection2), isEqual);
  }
}

/*
int isCooperativeBeforeLockTest(long peerID) {
    return 1;
}

int isCooperativeAfterLockTest(long peerID) {
  if(peerID == 3)
    return 0;
  else
    return 1;
}
*/

int isCooperativeBeforeLock() {
  return gsl_ran_discrete(r, uncoop_before_discrete);
}

int isCooperativeAfterLock() {
  return gsl_ran_discrete(r, uncoop_after_discrete);
}

uint64_t computeFee(uint64_t amountToForward, Policy policy) {
  uint64_t fee;
  fee = (policy.feeProportional*amountToForward) / 1000000;
  return policy.feeBase + fee;
}

void* dijkstraThread(void*arg) {
  Payment * payment;
  Array* hops;
  long paymentID;
  int *index;

  index = (int*) arg;


  while(1) {
    pthread_mutex_lock(&jobsMutex);
    jobs = pop(jobs, &paymentID);
    pthread_mutex_unlock(&jobsMutex);

    if(paymentID==-1) return NULL;

    pthread_mutex_lock(&peersMutex);
    payment = arrayGet(payments, paymentID);
    pthread_mutex_unlock(&peersMutex);

    printf("DIJKSTRA %ld\n", payment->ID);

    hops = dijkstraP(payment->sender, payment->receiver, payment->amount, payment->ignoredPeers,
                     payment->ignoredChannels, *index);

    //    printf("END DIJKSTRA %ld\n", payment->ID);

    //    pthread_mutex_lock(&pathsMutex);
    paths[payment->ID] = hops;
    //    pthread_mutex_unlock(&pathsMutex);

    /* pthread_mutex_lock(&(condMutex[payment->ID])); */
    /* condPaths[payment->ID] = 1; */
    /* pthread_cond_signal(&(condVar[payment->ID])); */
    /* pthread_mutex_unlock(&(condMutex[payment->ID])); */

  }

  return NULL;

}

unsigned int isAnyChannelClosed(Array* hops) {
  int i;
  Channel* channel;
  PathHop* hop;

  for(i=0;i<arrayLen(hops);i++) {
    hop = arrayGet(hops, i);
    channel = arrayGet(channels, hop->channel);
    if(channel->isClosed)
      return 1;
  }

  return 0;
}


void findRoute(Event *event) {
  Payment *payment;
  Array *pathHops;
  Route* route;
  int finalTimelock=9;
  Event* sendEvent;
  uint64_t nextEventTime;

  printf("FINDROUTE %ld\n", event->paymentID);

  payment = arrayGet(payments, event->paymentID);
  ++(payment->attempts);

  if(payment->startTime < 1)
    payment->startTime = simulatorTime;

  /*
  // dijkstra version
  pathHops = dijkstra(payment->sender, payment->receiver, payment->amount, payment->ignoredPeers,
                      payment->ignoredChannels);
*/  

  /* floydWarshall version
  if(payment->attempts == 0) {
    pathHops = getPath(payment->sender, payment->receiver);
  }
  else {
    pathHops = dijkstra(payment->sender, payment->receiver, payment->amount, payment->ignoredPeers,
                        payment->ignoredChannels);
                        }*/

  //dijkstra parallel OLD version
  /* if(payment->attempts > 0) */
  /*   pthread_create(&tid, NULL, &dijkstraThread, payment); */

  /* pthread_mutex_lock(&(condMutex[payment->ID])); */
  /* while(!condPaths[payment->ID]) */
  /*   pthread_cond_wait(&(condVar[payment->ID]), &(condMutex[payment->ID])); */
  /* condPaths[payment->ID] = 0; */
  /* pthread_mutex_unlock(&(condMutex[payment->ID])); */

  
  //dijkstra parallel NEW version
  if(payment->attempts==0) {
    pathHops = paths[payment->ID];
    if(pathHops!=NULL)
      if(isAnyChannelClosed(pathHops)) {
        pathHops = dijkstra(payment->sender, payment->receiver, payment->amount, payment->ignoredPeers,
                            payment->ignoredChannels);
        nDijkstra++;
      }
  }
  else {
    pathHops = dijkstra(payment->sender, payment->receiver, payment->amount, payment->ignoredPeers,
                        payment->ignoredChannels);
    nDijkstra++;
  }

  if(pathHops!=NULL)
    if(isAnyChannelClosed(pathHops)) {
    pathHops = dijkstra(payment->sender, payment->receiver, payment->amount, payment->ignoredPeers,
                                       payment->ignoredChannels);
    paths[payment->ID] = pathHops;
  }
  


  if (pathHops == NULL) {
    printf("No available path\n");
    payment->endTime = simulatorTime;
    statsUpdatePayments(payment);
    return;
  }

  route = transformPathIntoRoute(pathHops, payment->amount, finalTimelock);
  if(route==NULL) {
    printf("No available route\n");
    payment->endTime = simulatorTime;
    statsUpdatePayments(payment);
    return;
  }

  payment->route = route;

  nextEventTime = simulatorTime;
  sendEvent = createEvent(eventIndex, nextEventTime, SENDPAYMENT, payment->sender, event->paymentID );
  events = heapInsert(events, sendEvent, compareEvent);

}

RouteHop *getRouteHop(long peerID, Array *routeHops, int isSender) {
  RouteHop *routeHop;
  long i, index = -1;

  for (i = 0; i < arrayLen(routeHops); i++) {
    routeHop = arrayGet(routeHops, i);

    if (isSender && routeHop->pathHop->sender == peerID) {
      index = i;
      break;
    }

    if (!isSender && routeHop->pathHop->receiver == peerID) {
      index = i;
      break;
    }
  }

  if (index == -1)
    return NULL;

  return arrayGet(routeHops, index);
}

int checkPolicyForward( RouteHop* prevHop, RouteHop* currHop) {
  Policy policy;
  Channel* currChannel, *prevChannel;
  uint64_t fee;

  currChannel = arrayGet(channels, currHop->pathHop->channel);
  prevChannel = arrayGet(channels, prevHop->pathHop->channel);



  fee = computeFee(currHop->amountToForward,currChannel->policy);
  //the check should be: prevHop->amountToForward - fee != currHop->amountToForward
  if(prevHop->amountToForward - fee != currHop->amountToForward) {
    printf("ERROR: Fee not respected\n");
    printf("PrevHopAmount %ld - fee %ld != CurrHopAmount %ld\n", prevHop->amountToForward, fee, currHop->amountToForward);
    printHop(currHop);
    return 0;
  }

  if(prevHop->timelock - prevChannel->policy.timelock != currHop->timelock) {
    printf("ERROR: Timelock not respected\n");
    printf("PrevHopTimelock %d - policyTimelock %d != currHopTimelock %d \n",prevHop->timelock, policy.timelock, currHop->timelock);
    printHop(currHop);
    return 0;
  }

  return 1;
}

//TODO: uniformare tipi di variabili d'appoggio da usare nelle seguenti tre funzioni
// in particolare evitare tutte le variabili che non siano puntatori, perche e' rischioso
// passarne poi l'indirizzo

void sendPayment(Event* event) {
  Payment* payment;
  uint64_t nextEventTime;
  Route* route;
  RouteHop* firstRouteHop;
  Channel* nextChannel;
  ChannelInfo* channelInfo;
  Event* nextEvent;
  EventType eventType;
  Peer* peer;

  //  printf("SEND PAYMENT %ld\n", event->paymentID);

  payment = arrayGet(payments, event->paymentID);
  peer = arrayGet(peers, event->peerID);
  route = payment->route;
  firstRouteHop = arrayGet(route->routeHops, 0);
  nextChannel = arrayGet(channels, firstRouteHop->pathHop->channel);
  channelInfo = arrayGet(channelInfos, nextChannel->channelInfoID);

  if(!isPresent(nextChannel->ID, peer->channel)) {
    printf("Channel %ld has been closed\n", nextChannel->ID);
    nextEventTime = simulatorTime;
    nextEvent = createEvent(eventIndex, nextEventTime, FINDROUTE, event->peerID, event->paymentID );
    events = heapInsert(events, nextEvent, compareEvent);
    return;
  }


  if(firstRouteHop->amountToForward > nextChannel->balance) {
    printf("Not enough balance in channel %ld\n", nextChannel->ID);
    payment->ignoredChannels = arrayInsert(payment->ignoredChannels, &(nextChannel->ID));
    nextEventTime = simulatorTime;
    nextEvent = createEvent(eventIndex, nextEventTime, FINDROUTE, event->peerID, event->paymentID );
    events = heapInsert(events, nextEvent, compareEvent);
    return;
  }

  nextChannel->balance -= firstRouteHop->amountToForward;

  //  printf("Peer %ld, balance %lf\n", event->peerID, forwardChannel->balance);


  //TODO: creare funzione generateForwardEvent che ha tutte le seguenti righe di codice fino alla fine
  eventType = firstRouteHop->pathHop->receiver == payment->receiver ? RECEIVEPAYMENT : FORWARDPAYMENT;
  nextEventTime = simulatorTime + channelInfo->latency;
  nextEvent = createEvent(eventIndex, nextEventTime, eventType, firstRouteHop->pathHop->receiver, event->paymentID );
  events = heapInsert(events, nextEvent, compareEvent);
}

void forwardPayment(Event *event) {
  Payment* payment;
  Route* route;
  RouteHop* nextRouteHop, *previousRouteHop;
  long  nextPeerID, prevPeerID;
  EventType eventType;
  Event* nextEvent;
  uint64_t nextEventTime;
  ChannelInfo *prevChannelInfo, *nextChannelInfo;
  Channel* prevChannel, *nextChannel;
  int isPolicyRespected;
  Peer* peer;

  //  printf("FORWARD PAYMENT %ld\n", event->paymentID);

  payment = arrayGet(payments, event->paymentID);
  peer = arrayGet(peers, event->peerID);
  route = payment->route;
  nextRouteHop=getRouteHop(peer->ID, route->routeHops, 1);
  previousRouteHop = getRouteHop(peer->ID, route->routeHops, 0);
  prevChannel = arrayGet(channels, previousRouteHop->pathHop->channel);
  nextChannel = arrayGet(channels, nextRouteHop->pathHop->channel);
  prevChannelInfo = arrayGet(channelInfos, prevChannel->channelInfoID);
  nextChannelInfo = arrayGet(channelInfos, nextChannel->channelInfoID);

  if(nextRouteHop == NULL || previousRouteHop == NULL) {
    printf("ERROR: no route hop\n");
    return;
  }

  if(!isPresent(nextRouteHop->pathHop->channel, peer->channel)) {
    printf("Channel %ld has been closed\n", nextRouteHop->pathHop->channel);
    prevPeerID = previousRouteHop->pathHop->sender;
    eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    nextEventTime = simulatorTime + prevChannelInfo->latency;
    nextEvent = createEvent(eventIndex, nextEventTime, eventType, prevPeerID, event->paymentID);
    events = heapInsert(events, nextEvent, compareEvent);
    return;
  }

  

  if(!isCooperativeBeforeLock()){
    printf("Peer %ld is not cooperative before lock\n", event->peerID);
    payment->isAPeerUncoop = 1;
    payment->ignoredPeers = arrayInsert(payment->ignoredPeers, &(event->peerID));
    prevPeerID = previousRouteHop->pathHop->sender;
    eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    nextEventTime = simulatorTime + prevChannelInfo->latency + FAULTYLATENCY;
    nextEvent = createEvent(eventIndex, nextEventTime, eventType, prevPeerID, event->paymentID );
    events = heapInsert(events, nextEvent, compareEvent);
    return;
  }


  

  if(!isCooperativeAfterLock()) {
    printf("Peer %ld is not cooperative after lock on channel %ld\n", event->peerID, prevChannel->ID);
    payment->isAPeerUncoop = 1;
    closeChannel(prevChannel->channelInfoID);
    payment->ignoredChannels = arrayInsert(payment->ignoredChannels, &(prevChannel->ID));

    statsUpdateLockedFundCost(route->routeHops, previousRouteHop->pathHop->channel);

    prevPeerID = previousRouteHop->pathHop->sender;
    eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    nextEventTime =  simulatorTime + previousRouteHop->timelock*10*60000; //10 minutes (expressed in milliseconds) per block
    nextEvent = createEvent(eventIndex, nextEventTime, eventType, prevPeerID, event->paymentID );
    events = heapInsert(events, nextEvent, compareEvent);
    return;
  }



  isPolicyRespected = checkPolicyForward(previousRouteHop, nextRouteHop);
  if(!isPolicyRespected) return;

  if(nextRouteHop->amountToForward > nextChannel->balance ) {
    printf("Not enough balance in channel %ld\n", nextChannel->ID);
    payment->ignoredChannels = arrayInsert(payment->ignoredChannels, &(nextChannel->ID));
    prevPeerID = previousRouteHop->pathHop->sender;
    eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    nextEventTime = simulatorTime + prevChannelInfo->latency;
    nextEvent = createEvent(eventIndex, nextEventTime, eventType, prevPeerID, event->paymentID );
    events = heapInsert(events, nextEvent, compareEvent);
    return;
  }
  nextChannel->balance -= nextRouteHop->amountToForward;

  //  printf("Peer %ld, balance %lf\n", peerID, forwardChannel->balance);

  nextPeerID = nextRouteHop->pathHop->receiver;
  eventType = nextPeerID == payment->receiver ? RECEIVEPAYMENT : FORWARDPAYMENT;
  nextEventTime = simulatorTime + nextChannelInfo->latency;
  nextEvent = createEvent(eventIndex, nextEventTime, eventType, nextPeerID, event->paymentID );
  events = heapInsert(events, nextEvent, compareEvent);

}

void receivePayment(Event* event ) {
  long peerID, prevPeerID;
  Route* route;
  Payment* payment;
  RouteHop* lastRouteHop;
  Channel* forwardChannel,*backwardChannel;
  ChannelInfo* channelInfo;
  Event* nextEvent;
  EventType eventType;
  uint64_t nextEventTime;
  Peer* peer;

  //  printf("RECEIVE PAYMENT %ld\n", event->paymentID);
  peerID = event->peerID;
  peer = arrayGet(peers, peerID);
  payment = arrayGet(payments, event->paymentID);
  route = payment->route;

  lastRouteHop = arrayGet(route->routeHops, arrayLen(route->routeHops) - 1);
  forwardChannel = arrayGet(channels, lastRouteHop->pathHop->channel);
  backwardChannel = arrayGet(channels, forwardChannel->otherChannelDirectionID);
  channelInfo = arrayGet(channelInfos, forwardChannel->channelInfoID);

  backwardChannel->balance += lastRouteHop->amountToForward;

  if(!isCooperativeBeforeLock()){
    printf("Peer %ld is not cooperative before lock\n", event->peerID);
    payment->isAPeerUncoop = 1;
    payment->ignoredPeers = arrayInsert(payment->ignoredPeers, &(event->peerID));
    prevPeerID = lastRouteHop->pathHop->sender;
    eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    nextEventTime = simulatorTime + channelInfo->latency + FAULTYLATENCY;
    nextEvent = createEvent(eventIndex, nextEventTime, eventType, prevPeerID, event->paymentID );
    events = heapInsert(events, nextEvent, compareEvent);
    return;
  }



  if(peer->withholdsR) {
    printf("Peer %ld withholds R on channel %ld\n", event->peerID, backwardChannel->ID);
    payment->isAPeerUncoop = 1;
    closeChannel(backwardChannel->channelInfoID);
    payment->ignoredChannels = arrayInsert(payment->ignoredChannels, &(forwardChannel->ID));

    prevPeerID = lastRouteHop->pathHop->sender;
    eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    nextEventTime = simulatorTime + lastRouteHop->timelock*10*60000;
    nextEvent = createEvent(eventIndex, nextEventTime, eventType, prevPeerID, event->paymentID );
    events = heapInsert(events, nextEvent, compareEvent);
    return;
  }



  payment->isSuccess = 1;
  //  printf("Peer %ld, balance %lf\n", peerID, backwardChannel->balance);

  prevPeerID = lastRouteHop->pathHop->sender;
  eventType = prevPeerID == payment->sender ? RECEIVESUCCESS : FORWARDSUCCESS;
  nextEventTime = simulatorTime + channelInfo->latency;
  nextEvent = createEvent(eventIndex, nextEventTime, eventType, prevPeerID, event->paymentID);
  events = heapInsert(events, nextEvent, compareEvent);
}

//TODO: forse ha senso memorizzare nel peer tutti i payment ID che lo interessano (sia come sender che come receiver che come hop);
//     questo puo' servire sia per motivi statistici che anche per debugging: controllare cioe che il peer non riceva una richiesta
//     di settle o fail per un pagamento che non lo riguarda.

void forwardSuccess(Event* event) {
  RouteHop* prevHop, *nextHop;
  Payment* payment;
  Channel* forwardChannel, * backwardChannel, *nextChannel;
  ChannelInfo *prevChannelInfo;
  long prevPeerID;
  Event* nextEvent;
  EventType eventType;
  Peer* peer;
  uint64_t nextEventTime;

  //  printf("FORWARD SUCCESS  %ld\n", event->paymentID);

  payment = arrayGet(payments, event->paymentID);
  prevHop = getRouteHop(event->peerID, payment->route->routeHops, 0);
  nextHop = getRouteHop(event->peerID, payment->route->routeHops, 1);
  nextChannel = arrayGet(channels, nextHop->pathHop->channel);
  forwardChannel = arrayGet(channels, prevHop->pathHop->channel);
  backwardChannel = arrayGet(channels, forwardChannel->otherChannelDirectionID);
  prevChannelInfo = arrayGet(channelInfos, forwardChannel->channelInfoID);
  peer = arrayGet(peers, event->peerID);
 

  if(!isPresent(backwardChannel->ID, peer->channel)) {
    printf("Channel %ld is not present\n", prevHop->pathHop->channel);
    prevPeerID = prevHop->pathHop->sender;
    eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    nextEventTime = simulatorTime + prevChannelInfo->latency;
    nextEvent = createEvent(eventIndex, nextEventTime, eventType, prevPeerID, event->paymentID);
    events = heapInsert(events, nextEvent, compareEvent);
    return;
  }



  if(!isCooperativeAfterLock()) {
    printf("Peer %ld is not cooperative after lock on channel %ld\n", event->peerID, nextChannel->ID);
    payment->isAPeerUncoop = 1;
    closeChannel(nextChannel->channelInfoID);
    payment->ignoredChannels = arrayInsert(payment->ignoredChannels, &(nextChannel->ID));

    prevPeerID = prevHop->pathHop->sender;
    eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    nextEventTime = simulatorTime + prevHop->timelock*10*60000;
    nextEvent = createEvent(eventIndex, nextEventTime, eventType, prevPeerID, event->paymentID );
    events = heapInsert(events, nextEvent, compareEvent);

    return;
  }



  backwardChannel->balance += prevHop->amountToForward;

  //  printf("Peer %ld, balance %lf\n", event->peerID, backwardChannel->balance);

  prevPeerID = prevHop->pathHop->sender;
  eventType = prevPeerID == payment->sender ? RECEIVESUCCESS : FORWARDSUCCESS;
  nextEventTime = simulatorTime + prevChannelInfo->latency;
  nextEvent = createEvent(eventIndex, nextEventTime, eventType, prevPeerID, event->paymentID);
  events = heapInsert(events, nextEvent, compareEvent);
}

void receiveSuccess(Event* event){
  Payment *payment;
  printf("RECEIVE SUCCESS %ld\n", event->paymentID);
  payment = arrayGet(payments, event->paymentID);
  payment->endTime = simulatorTime;
  statsUpdatePayments(payment);
}


//TODO: uniformare tutti i vari RouteHop a next previous
void forwardFail(Event* event) {
  Payment* payment;
  RouteHop* nextHop, *prevHop;
  Channel* nextChannel, *prevChannel;
  ChannelInfo *prevChannelInfo;
  long prevPeerID;
  Event* nextEvent;
  EventType eventType;
  Peer* peer;
  uint64_t nextEventTime;

  //  printf("FORWARD FAIL %ld\n", event->paymentID);

  peer = arrayGet(peers, event->peerID); 
  payment = arrayGet(payments, event->paymentID);
  nextHop = getRouteHop(event->peerID, payment->route->routeHops, 1);
  nextChannel = arrayGet(channels, nextHop->pathHop->channel);

  if(isPresent(nextChannel->ID, peer->channel)) {
    nextChannel->balance += nextHop->amountToForward;
  }
  else
    printf("Channel %ld is not present\n", nextHop->pathHop->channel);

  prevHop = getRouteHop(event->peerID, payment->route->routeHops, 0);
  prevChannel = arrayGet(channels, prevHop->pathHop->channel);
  prevChannelInfo = arrayGet(channelInfos, prevChannel->channelInfoID);
  prevPeerID = prevHop->pathHop->sender;
  eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
  nextEventTime = simulatorTime + prevChannelInfo->latency;
  nextEvent = createEvent(eventIndex, nextEventTime, eventType, prevPeerID, event->paymentID);
  events = heapInsert(events, nextEvent, compareEvent);
}


void receiveFail(Event* event) {
  Payment* payment;
  RouteHop* firstHop;
  Channel* nextChannel;
  Event* nextEvent;
  Peer* peer;
  uint64_t nextEventTime;

  printf("RECEIVE FAIL %ld\n", event->paymentID);

  payment = arrayGet(payments, event->paymentID);
  firstHop = arrayGet(payment->route->routeHops, 0);
  nextChannel = arrayGet(channels, firstHop->pathHop->channel);
  peer = arrayGet(peers, event->peerID);

  if(isPresent(nextChannel->ID, peer->channel))
    nextChannel->balance += firstHop->amountToForward;
  else
    printf("Channel %ld is not present\n", nextChannel->ID);

  if(payment->isSuccess == 1 ) {
    payment->endTime = simulatorTime;
    statsUpdatePayments(payment);
    return; //it means that money actually arrived to the destination but a peer was not cooperative when forwarding the success
  } 

  nextEventTime = simulatorTime;
  nextEvent = createEvent(eventIndex, nextEventTime, FINDROUTE, payment->sender, payment->ID);
  events = heapInsert(events, nextEvent, compareEvent);

}

/*
  long getChannelIndex(Peer* peer) {
  long index=-1, i;
  for(i=0; i<peer->channelSize; i++) {
  if(peer->channel[i]==-1) return i;
  }
  return -1;
  }
*/
