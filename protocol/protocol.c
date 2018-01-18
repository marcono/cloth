#include <stdlib.h>
#include <stdio.h>
#include <gsl/gsl_rng.h>
#include "../gc-7.2/include/gc.h"
#include "protocol.h"
#include "../utils/array.h"
#include "../utils/hashTable.h"
#include "../utils/heap.h"
#include "../simulator/initialize.h"
#include "findRoute.h"
#include "../simulator/event.h"
#include "../simulator/stats.h"

#define MAXSATOSHI 1E15 //1 million bitcoin
#define MAXTIMELOCK 100
#define MINTIMELOCK 10
#define MAXFEEBASE 1000
#define MINFEEBASE 100
#define MAXFEEPROP 10
#define MINFEEPROP 1
#define MAXLATENCY 10
#define MINLATENCY 1


//TODO: creare ID randomici e connettere peer "vicini" usando il concetto
// di vicinanza di chord; memorizzare gli id dei peer in un Array di long
// per avere facile accesso a tutti i peer nel momento della creazione dei canali
// (per controllare invece se un certo ID e' gia' stato usato, usare direttamente l'hashtable dei peer
// perche' e' piu' efficiente)
// CONTROLLARE SE QUESTO POTREBBE ESSERE UN PROBLEMA PER GLI ALGORITMI DI ROUTING

long channelIndex, peerIndex, channelInfoIndex, paymentIndex;
HashTable* peers;
HashTable* channels;
HashTable* channelInfos;
HashTable* payments;
double pUncoopBeforeLock, pUncoopAfterLock;

Peer* createPeer(long ID, long channelsSize) {
  Peer* peer;

  peer = GC_MALLOC(sizeof(Peer));
  peer->ID=ID;
  peer->channel = arrayInitialize(channelsSize);
  peer->initialFunds = 0.0;
  peer->remainingFunds = 0.0;
  peer->withholdsR = 0;

  peerIndex++;

  return peer;
}

ChannelInfo* createChannelInfo(long ID, long peer1, long peer2, double latency) {
  ChannelInfo* channelInfo;

  channelInfo = GC_MALLOC(sizeof(ChannelInfo));
  channelInfo->ID = ID;
  channelInfo->peer1 = peer1;
  channelInfo->peer2 = peer2;
  channelInfo->latency = latency;
  channelInfo->capacity = 0.0;

  channelInfoIndex++;

  return channelInfo;
}

double computeChannelBalance(Peer *peer) {
  double channelBalance;

  channelBalance = peer->initialFunds/4; //TODO: al posto di 4, il numero di canali per peer o altro
  if(peer->remainingFunds - channelBalance < 0) {
    channelBalance = peer->remainingFunds;
    peer->remainingFunds=0.0;
  }
  else
    peer->remainingFunds -= channelBalance;

  return channelBalance;
}

Channel* createChannel(long ID, long channelInfoID, long counterparty, Policy policy){
  Channel* channel;

  channel = GC_MALLOC(sizeof(Channel));
  channel->ID = ID;
  channel->channelInfoID = channelInfoID;
  channel->counterparty = counterparty;
  channel->policy = policy;
  channel->balance = 0.0;

  channelIndex++;

  return channel;
}

Payment* createPayment(long ID, long sender, long receiver, double amount) {
  Payment * p;

  p = GC_MALLOC(sizeof(Payment));
  p->ID=ID;
  p->sender= sender;
  p->receiver = receiver;
  p->amount = amount;
  p->route = NULL;
  p->ignoredChannels = arrayInitialize(5);
  p->ignoredPeers = arrayInitialize(5);
  p->isSuccess = 0;
  p->isAPeerUncoop = 0;
  p->startTime = -1.0;
  p->endTime = -1.0;

  paymentIndex++;

  return p;
}

void connectPeers(long peerID1, long peerID2) {
  Peer* peer1, *peer2;
  Policy policy1, policy2;
  Channel* firstChannelDirection, *secondChannelDirection; //TODO: rinominare channelInfo->channel e channel->channelDirection
  ChannelInfo *channelInfo;
  double latency;
  gsl_rng *r;
  const gsl_rng_type * T;

  gsl_rng_env_setup();
  T = gsl_rng_default;
  r = gsl_rng_alloc (T);

  peer1 = hashTableGet(peers, peerID1);
  peer2 = hashTableGet(peers, peerID2);

  latency = (gsl_rng_uniform_int(r, MAXLATENCY - MINLATENCY) + MINLATENCY)/1.0E3;
  channelInfo = createChannelInfo(channelInfoIndex, peer1->ID, peer2->ID, latency);
  hashTablePut(channelInfos, channelInfo->ID, channelInfo);


  policy1.feeBase = (gsl_rng_uniform_int(r, MAXFEEBASE - MINFEEBASE) + MINFEEBASE)/1.0E3;
  policy1.feeProportional = (gsl_rng_uniform_int(r, MAXFEEPROP-MINFEEPROP)+MINFEEPROP)/1.0E6;
  policy1.timelock = gsl_rng_uniform_int(r, MAXTIMELOCK-MINTIMELOCK)+MINTIMELOCK;
  firstChannelDirection = createChannel(channelIndex, channelInfo->ID, peer2->ID, policy1);
  firstChannelDirection->balance = computeChannelBalance(peer1);
  hashTablePut(channels, firstChannelDirection->ID, firstChannelDirection);
  peer1->channel = arrayInsert(peer1->channel, &(firstChannelDirection->ID));
  channelInfo->channelDirection1 = firstChannelDirection->ID;

  policy2.feeBase = (gsl_rng_uniform_int(r, MAXFEEBASE - MINFEEBASE) + MINFEEBASE)/1.0E3;
  policy2.feeProportional = (gsl_rng_uniform_int(r, MAXFEEPROP-MINFEEPROP)+MINFEEPROP)/1.0E6;
  policy2.timelock = gsl_rng_uniform_int(r, MAXTIMELOCK-MINTIMELOCK)+MINTIMELOCK;
  secondChannelDirection = createChannel(channelIndex, channelInfo->ID, peer1->ID, policy2);
  secondChannelDirection->balance = computeChannelBalance(peer2);
  hashTablePut(channels,secondChannelDirection->ID, secondChannelDirection);
  peer2->channel =arrayInsert(peer2->channel, &(secondChannelDirection->ID));
  channelInfo->channelDirection2 = secondChannelDirection->ID;

  firstChannelDirection->otherChannelDirectionID = secondChannelDirection->ID;
  secondChannelDirection->otherChannelDirectionID = firstChannelDirection->ID;

  channelInfo->capacity = firstChannelDirection->balance + secondChannelDirection->balance;

  //  gsl_rng_free(r);

}

void computePeersInitialFunds(double gini) {
  long i;
  Peer* peer;

  for(i=0; i<peerIndex; i++) {
    peer = hashTableGet(peers, i);
    peer->initialFunds = MAXSATOSHI/peerIndex;
    peer->remainingFunds = peer->initialFunds;
  }
}

void initializeTopology(long nPeers, long nChannels, double RWithholding, double gini) {
  long i, j, RWithholdingPeerID, nRWithholdingPeers, counterpartyID;
  Peer* peer, *counterparty;
  gsl_rng *r;
  const gsl_rng_type * T;

  gsl_rng_env_setup();

  T = gsl_rng_default;
  r = gsl_rng_alloc (T);

  for(i=0; i<nPeers; i++){
    peer = createPeer(peerIndex, nChannels);
    hashTablePut(peers, peer->ID, peer);
  }

  computePeersInitialFunds(gini);

  nRWithholdingPeers = nPeers*RWithholding;
  for(i=0; i < nRWithholdingPeers ;i++) {
    RWithholdingPeerID = gsl_rng_uniform_int(r,peerIndex);
    peer = hashTableGet(peers, RWithholdingPeerID);
    peer->withholdsR = 1;
  }

  for(i=0; i<peerIndex; i++) {
    peer = hashTableGet(peers, i);
    for(j=0; j<nChannels && (arrayLen(peer->channel) < nChannels); j++){

      do {
        counterpartyID = gsl_rng_uniform_int(r,peerIndex);
      }while(counterpartyID==peer->ID);

      counterparty = hashTableGet(peers, counterpartyID);
      if(arrayLen(counterparty->channel)>=nChannels) continue;

      connectPeers(peer->ID, counterparty->ID);
    }
  }

  gsl_rng_free(r);

}

void initializeProtocolData(long nPeers, long nChannels, double pUncoopBefore, double pUncoopAfter, double RWithholding, double gini) {

  channelIndex = peerIndex = channelInfoIndex = paymentIndex = 0;

  pUncoopBeforeLock = pUncoopBefore;
  pUncoopAfterLock = pUncoopAfter;


  peers = hashTableInitialize(1000);
  channels = hashTableInitialize(1000);
  channelInfos= hashTableInitialize(1000);
  payments = hashTableInitialize(1000);

  initializeTopology(nPeers, nChannels, RWithholding, gini);

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

  channelInfo = hashTableGet(channelInfos, channelInfoID);

  for(i = 0; i < peerIndex; i++) {
    peer = hashTableGet(peers, i);
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
  return 1;
}

int isCooperativeAfterLock() {
  return 1;
}

double computeFee(double amountToForward, Policy policy) {
  return policy.feeBase + policy.feeProportional*amountToForward;
}

void findRoute(Event *event) {
  Payment *payment;
  Array *pathHops;
  Route* route;
  int finalTimelock=9;
  Event* sendEvent;
  double nextEventTime;

  printf("FINDROUTE %ld\n", event->paymentID);

  payment = hashTableGet(payments, event->paymentID);
  if(payment->startTime < 0) payment->startTime = simulatorTime;

  pathHops = dijkstra(payment->sender, payment->receiver, payment->amount, payment->ignoredPeers,
                      payment->ignoredChannels);
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
  double fee, expectedZero;

  currChannel = hashTableGet(channels, currHop->pathHop->channel);
  prevChannel = hashTableGet(channels, prevHop->pathHop->channel);



  fee = computeFee(currHop->amountToForward,currChannel->policy);
  expectedZero = (prevHop->amountToForward - fee) - currHop->amountToForward;
  //the check should be: prevHop->amountToForward - fee != currHop->amountToForward
  if(expectedZero > 0.0000000001) {
    printf("Error: Fee not respected\n");
    printf("prevHop amount %.3lf  - fee %.3lf != currHop amount %.3lf\n", prevHop->amountToForward, fee, currHop->amountToForward);
    return 0;
  }

  if(prevHop->timelock - prevChannel->policy.timelock != currHop->timelock) {
    printf("Error: Timelock not respected\n");
    printf("PrevHopTimelock %d - policyTimelock %d != currHopTimelock %d \n",prevHop->timelock, policy.timelock, currHop->timelock);
    return 0;
  }

  return 1;
}

//TODO: uniformare tipi di variabili d'appoggio da usare nelle seguenti tre funzioni
// in particolare evitare tutte le variabili che non siano puntatori, perche e' rischioso
// passarne poi l'indirizzo

void sendPayment(Event* event) {
  Payment* payment;
  double  newBalance, nextEventTime;
  Route* route;
  RouteHop* firstRouteHop;
  Channel* nextChannel;
  Event* nextEvent;
  EventType eventType;
  Peer* peer;

  printf("SEND PAYMENT %ld\n", event->paymentID);

  payment = hashTableGet(payments, event->paymentID);
  peer = hashTableGet(peers, event->peerID);
  route = payment->route;
  firstRouteHop = arrayGet(route->routeHops, 0);
  nextChannel = hashTableGet(channels, firstRouteHop->pathHop->channel);

  if(!isPresent(nextChannel->ID, peer->channel)) {
    printf("Channel %ld has been closed\n", nextChannel->ID);
    nextEventTime = simulatorTime;
    nextEvent = createEvent(eventIndex, nextEventTime, FINDROUTE, event->peerID, event->paymentID );
    events = heapInsert(events, nextEvent, compareEvent);
    return;
  }


  newBalance = nextChannel->balance - firstRouteHop->amountToForward;
  if(newBalance < 0) {
    printf("not enough balance\n");
    payment->ignoredChannels = arrayInsert(payment->ignoredChannels, &(nextChannel->ID));
    nextEventTime = simulatorTime;
    nextEvent = createEvent(eventIndex, nextEventTime, FINDROUTE, event->peerID, event->paymentID );
    events = heapInsert(events, nextEvent, compareEvent);
    return;
  }

  nextChannel->balance = newBalance;

  //  printf("Peer %ld, balance %lf\n", event->peerID, forwardChannel->balance);


  //TODO: creare funzione generateForwardEvent che ha tutte le seguenti righe di codice fino alla fine
  eventType = firstRouteHop->pathHop->receiver == payment->receiver ? RECEIVEPAYMENT : FORWARDPAYMENT;
  nextEventTime = simulatorTime + 0.1;
  nextEvent = createEvent(eventIndex, nextEventTime, eventType, firstRouteHop->pathHop->receiver, event->paymentID );
  events = heapInsert(events, nextEvent, compareEvent);
}

void forwardPayment(Event *event) {
  Payment* payment;
  Route* route;
  RouteHop* nextRouteHop, *previousRouteHop;
  long  nextPeerID, prevPeerID, nextChannelID;
  EventType eventType;
  Event* nextEvent;
  double newBalance, nextEventTime;
  Channel* prevChannel, *nextChannel;
  int isPolicyRespected;
  Peer* peer;

  printf("FORWARD PAYMENT %ld\n", event->paymentID);

  payment = hashTableGet(payments, event->paymentID);
  peer = hashTableGet(peers, event->peerID);
  route = payment->route;
  nextRouteHop=getRouteHop(peer->ID, route->routeHops, 1);
  previousRouteHop = getRouteHop(peer->ID, route->routeHops, 0);
  if(nextRouteHop == NULL || previousRouteHop == NULL) {
    printf("no route hop\n");
    return;
  }

  if(!isPresent(nextRouteHop->pathHop->channel, peer->channel)) {
    printf("Channel %ld has been closed\n", nextRouteHop->pathHop->channel);
    prevPeerID = previousRouteHop->pathHop->sender;
    eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    nextEventTime = simulatorTime + 0.1;
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
    nextEventTime = simulatorTime + 0.1;
    nextEvent = createEvent(eventIndex, nextEventTime, eventType, prevPeerID, event->paymentID );
    events = heapInsert(events, nextEvent, compareEvent);
    return;
  }


  

  if(!isCooperativeAfterLock() || (event->peerID==2 && event->paymentID==2) || (event->peerID==1 && event->paymentID==3)) {
    printf("Peer %ld is not cooperative after lock\n", event->peerID);
    payment->isAPeerUncoop = 1;
    prevChannel = hashTableGet(channels, previousRouteHop->pathHop->channel);
    closeChannel(prevChannel->channelInfoID);

    statsUpdateLockedFundCost(route->routeHops, previousRouteHop->pathHop->channel);

    prevPeerID = previousRouteHop->pathHop->sender;
    eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    nextEventTime =  simulatorTime + 0.1*previousRouteHop->timelock;
    nextEvent = createEvent(eventIndex, nextEventTime, eventType, prevPeerID, event->paymentID );
    events = heapInsert(events, nextEvent, compareEvent);
    return;
  }



  isPolicyRespected = checkPolicyForward(previousRouteHop, nextRouteHop);
  if(!isPolicyRespected) return;

  nextChannelID = nextRouteHop->pathHop->channel;
  nextChannel = hashTableGet(channels, nextChannelID);
  newBalance = nextChannel->balance - nextRouteHop->amountToForward;
  if(newBalance < 0) {
    printf("not enough balance\n");
    payment->ignoredChannels = arrayInsert(payment->ignoredChannels, &(nextChannel->ID));
    prevPeerID = previousRouteHop->pathHop->sender;
    eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    nextEventTime = simulatorTime + 0.1;
    nextEvent = createEvent(eventIndex, nextEventTime, eventType, prevPeerID, event->paymentID );
    events = heapInsert(events, nextEvent, compareEvent);
    return;
  }
  nextChannel->balance = newBalance;

  //  printf("Peer %ld, balance %lf\n", peerID, forwardChannel->balance);

  nextPeerID = nextRouteHop->pathHop->receiver;
  eventType = nextPeerID == payment->receiver ? RECEIVEPAYMENT : FORWARDPAYMENT;
  nextEventTime = simulatorTime + 0.1;
  nextEvent = createEvent(eventIndex, nextEventTime, eventType, nextPeerID, event->paymentID );
  events = heapInsert(events, nextEvent, compareEvent);

}

void receivePayment(Event* event ) {
  long peerID, prevPeerID;
  Route* route;
  Payment* payment;
  RouteHop* lastRouteHop;
  Channel* forwardChannel,*backwardChannel;
  Event* nextEvent;
  EventType eventType;
  double nextEventTime;

  printf("RECEIVE PAYMENT %ld\n", event->paymentID);
  peerID = event->peerID;
  payment = hashTableGet(payments, event->paymentID);
  route = payment->route;

  lastRouteHop = arrayGet(route->routeHops, arrayLen(route->routeHops) - 1);
  forwardChannel = hashTableGet(channels, lastRouteHop->pathHop->channel);
  backwardChannel = hashTableGet(channels, forwardChannel->otherChannelDirectionID);

  backwardChannel->balance += lastRouteHop->amountToForward;

  if(!isCooperativeBeforeLock()){
    printf("Peer %ld is not cooperative before lock\n", event->peerID);
    payment->isAPeerUncoop = 1;
    payment->ignoredPeers = arrayInsert(payment->ignoredPeers, &(event->peerID));
    prevPeerID = lastRouteHop->pathHop->sender;
    eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    nextEventTime = simulatorTime + 0.1;
    nextEvent = createEvent(eventIndex, nextEventTime, eventType, prevPeerID, event->paymentID );
    events = heapInsert(events, nextEvent, compareEvent);
    return;
  }



  if(!isCooperativeAfterLock()) {
    printf("Peer %ld is not cooperative after lock\n", event->peerID);
    payment->isAPeerUncoop = 1;
    closeChannel(backwardChannel->channelInfoID);

    prevPeerID = lastRouteHop->pathHop->sender;
    eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    nextEventTime = simulatorTime + 0.1*lastRouteHop->timelock;
    nextEvent = createEvent(eventIndex, nextEventTime, eventType, prevPeerID, event->paymentID );
    events = heapInsert(events, nextEvent, compareEvent);
    return;
  }



  payment->isSuccess = 1;
  //  printf("Peer %ld, balance %lf\n", peerID, backwardChannel->balance);

  prevPeerID = lastRouteHop->pathHop->sender;
  eventType = prevPeerID == payment->sender ? RECEIVESUCCESS : FORWARDSUCCESS;
  nextEventTime = simulatorTime + 0.1;
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
  long prevPeerID;
  Event* nextEvent;
  EventType eventType;
  Peer* peer;
  double nextEventTime;

  printf("FORWARD SUCCESS  %ld\n", event->paymentID);

  payment = hashTableGet(payments, event->paymentID);
  prevHop = getRouteHop(event->peerID, payment->route->routeHops, 0);
  forwardChannel = hashTableGet(channels, prevHop->pathHop->channel);
  backwardChannel = hashTableGet(channels, forwardChannel->otherChannelDirectionID);
  peer = hashTableGet(peers, event->peerID);

  if(!isPresent(backwardChannel->ID, peer->channel)) {
    printf("Channel %ld is not present\n", prevHop->pathHop->channel);
    prevPeerID = prevHop->pathHop->sender;
    eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    nextEventTime = simulatorTime + 0.1;
    nextEvent = createEvent(eventIndex, nextEventTime, eventType, prevPeerID, event->paymentID);
    events = heapInsert(events, nextEvent, compareEvent);
    return;
  }



  if(!isCooperativeAfterLock()) {
    printf("Peer %ld is not cooperative after lock\n", event->peerID);
    payment->isAPeerUncoop = 1;
    nextHop = getRouteHop(event->peerID, payment->route->routeHops, 1);
    nextChannel = hashTableGet(channels, nextHop->pathHop->channel);
    closeChannel(nextChannel->channelInfoID);

    prevPeerID = prevHop->pathHop->sender;
    eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    nextEventTime = simulatorTime + 0.1*prevHop->timelock;
    nextEvent = createEvent(eventIndex, nextEventTime, eventType, prevPeerID, event->paymentID );
    events = heapInsert(events, nextEvent, compareEvent);

    return;
  }



  backwardChannel->balance += prevHop->amountToForward;

  //  printf("Peer %ld, balance %lf\n", event->peerID, backwardChannel->balance);

  prevPeerID = prevHop->pathHop->sender;
  eventType = prevPeerID == payment->sender ? RECEIVESUCCESS : FORWARDSUCCESS;
  nextEventTime = simulatorTime + 0.1;
  nextEvent = createEvent(eventIndex, nextEventTime, eventType, prevPeerID, event->paymentID);
  events = heapInsert(events, nextEvent, compareEvent);
}

void receiveSuccess(Event* event){
  Payment *payment;
  printf("RECEIVE SUCCESS %ld\n", event->paymentID);
  payment = hashTableGet(payments, event->paymentID);
  payment->endTime = simulatorTime;
  statsUpdatePayments(payment);
}


//TODO: uniformare tutti i vari RouteHop a next previous
void forwardFail(Event* event) {
  Payment* payment;
  RouteHop* nextHop, *prevHop;
  Channel* nextChannel;
  long prevPeerID;
  Event* nextEvent;
  EventType eventType;
  Peer* peer;
  double nextEventTime;

  printf("FORWARD FAIL %ld\n", event->paymentID);

  peer = hashTableGet(peers, event->peerID); 
  payment = hashTableGet(payments, event->paymentID);
  nextHop = getRouteHop(event->peerID, payment->route->routeHops, 1);
  nextChannel = hashTableGet(channels, nextHop->pathHop->channel);

  if(isPresent(nextChannel->ID, peer->channel)) {
    nextChannel->balance += nextHop->amountToForward;
  }
  else
    printf("Channel %ld is not present\n", nextHop->pathHop->channel);

  prevHop = getRouteHop(event->peerID, payment->route->routeHops, 0);
  prevPeerID = prevHop->pathHop->sender;
  eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
  nextEventTime = simulatorTime + 0.1;
  nextEvent = createEvent(eventIndex, nextEventTime, eventType, prevPeerID, event->paymentID);
  events = heapInsert(events, nextEvent, compareEvent);
}


void receiveFail(Event* event) {
  Payment* payment;
  RouteHop* firstHop;
  Channel* nextChannel;
  Event* nextEvent;
  Peer* peer;
  double nextEventTime;

  printf("RECEIVE FAIL %ld\n", event->paymentID);

  payment = hashTableGet(payments, event->paymentID);
  firstHop = arrayGet(payment->route->routeHops, 0);
  nextChannel = hashTableGet(channels, firstHop->pathHop->channel);
  peer = hashTableGet(peers, event->peerID);

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
