#include <stdlib.h>
#include <stdio.h>
#include "../gc-7.2/include/gc.h"
#include "protocol.h"
#include "../utils/array.h"
#include "../utils/hashTable.h"
#include "../utils/heap.h"
#include "../simulator/initialize.h"
#include "findRoute.h"
#include "../simulator/event.h"

//TODO: creare ID randomici e connettere peer "vicini" usando il concetto
// di vicinanza di chord; memorizzare gli id dei peer in un Array di long
// per avere facile accesso a tutti i peer nel momento della creazione dei canali
// (per controllare invece se un certo ID e' gia' stato usato, usare direttamente l'hashtable dei peer
// perche' e' piu' efficiente)
// CONTROLLARE SE QUESTO POTREBBE ESSERE UN PROBLEMA PER GLI ALGORITMI DI ROUTING

long channelIndex, peerIndex, channelInfoIndex, paymentIndex;
long nPeers;
long nChannels;
HashTable* peers;
HashTable* channels;
HashTable* channelInfos;
HashTable* payments;


void initializeProtocolData(long nP, long nC) {
  nPeers = nP;
  nChannels = nC;

  channelIndex = peerIndex = channelInfoIndex = paymentIndex = 0;

  peers = hashTableInitialize(2);
  channels = hashTableInitialize(2);
  channelInfos= hashTableInitialize(2);
  payments = hashTableInitialize(2);


}

Peer* createPeer(long ID, long channelsSize) {
  Peer* peer;

  peer = GC_MALLOC(sizeof(Peer));
  peer->ID=ID;
  peer->channelsSize = channelsSize;
  peer->channel = arrayInitialize(peer->channelsSize);

  peerIndex++;

  return peer;
}

ChannelInfo* createChannelInfo(long ID, long peer1, long peer2, double capacity) {
  ChannelInfo* channelInfo;

  channelInfo = GC_MALLOC(sizeof(ChannelInfo));
  channelInfo->ID = ID;
  channelInfo->peer1 = peer1;
  channelInfo->peer2 = peer2;
  channelInfo->capacity = capacity;

  channelInfoIndex++;

  return channelInfo;
}

Channel* createChannel(long ID, long channelInfoID, long counterparty, Policy policy, double balance){
  Channel* channel;

  channel = GC_MALLOC(sizeof(Channel));
  channel->ID = ID;
  channel->channelInfoID = channelInfoID;
  channel->counterparty = counterparty;
  channel->policy = policy;
  channel->balance = balance;

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

  paymentIndex++;

  return p;
}

void connectPeers(long peerID1, long peerID2) {
  Peer* peer1, *peer2;
  Policy policy;
  Channel* firstChannelDirection, *secondChannelDirection; //TODO: rinominare channelInfo->channel e channel->channelDirection
  ChannelInfo *channelInfo;
  double capacity;

  policy.feeBase = 0.1;
  policy.feeProportional = 0.0;
  policy.timelock = 1;

  peer1 = hashTableGet(peers, peerID1);
  peer2 = hashTableGet(peers, peerID2);

  capacity = 5.0;

  channelInfo = createChannelInfo(channelInfoIndex, peer1->ID, peer2->ID, capacity);
  hashTablePut(channelInfos, channelInfo->ID, channelInfo);


  //TODO: meglio mettere il settaggio della balance del canale (sempre pari a capacity/2) dentro
  //la funzione create channel, magari passando come parametro direttamente l'oggetto channelInfos (cosi si prende ID e capacity)
  firstChannelDirection = createChannel(channelIndex, channelInfo->ID, peer2->ID, policy, channelInfo->capacity*0.5);
  hashTablePut(channels, firstChannelDirection->ID, firstChannelDirection);
  peer1->channel = arrayInsert(peer1->channel, &(firstChannelDirection->ID));
  channelInfo->channelDirection1 = firstChannelDirection->ID;


  secondChannelDirection = createChannel(channelIndex, channelInfo->ID, peer1->ID, policy, channelInfo->capacity*0.5 );
  hashTablePut(channels,secondChannelDirection->ID, secondChannelDirection);
  peer2->channel =arrayInsert(peer2->channel, &(secondChannelDirection->ID));
  channelInfo->channelDirection2 = secondChannelDirection->ID;

  firstChannelDirection->otherChannelDirectionID = secondChannelDirection->ID;
  secondChannelDirection->otherChannelDirectionID = firstChannelDirection->ID;
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


int isCooperativeBeforeLockTest(long peerID) {
    return 1;
}

int isCooperativeAfterLockTest(long peerID) {
  if(peerID == 2)
    return 0;
  else
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

  printf("FINDROUTE %ld\n", event->paymentID);

  payment = hashTableGet(payments, event->paymentID);

  pathHops = dijkstra(payment->sender, payment->receiver, payment->amount, payment->ignoredPeers,
                      payment->ignoredChannels);
  if (pathHops == NULL) {
    printf("No available path\n");
    return;
  }

  route = transformPathIntoRoute(pathHops, payment->amount, finalTimelock);
  if(route==NULL) {
    printf("No available route\n");
    return;
  }

  payment->route = route;

  sendEvent = createEvent(eventIndex, simulatorTime, SENDPAYMENT, payment->sender, event->paymentID );
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
  long  nextPeerID, forwardChannelID, routeLen;
  double amountToForward, newBalance;
  Route* route;
  PathHop* firstPathHop;
  RouteHop* firstRouteHop;
  Array* routeHops;
  Channel* forwardChannel;
  Event* nextEvent;
  EventType eventType;
  Peer* peer;

  printf("SEND PAYMENT %ld\n", event->paymentID);

  payment = hashTableGet(payments, event->paymentID);
  route = payment->route;
  routeHops = route->routeHops;
  routeLen = arrayLen(routeHops);

  firstRouteHop = arrayGet(routeHops, 0);
  firstPathHop = firstRouteHop->pathHop;
  //TODO: controllare che questo canale sia effettivamente tra i canali del peer: potrebbe capitare infatti che per
  //qualche motivo sia stato chiuso; in questo caso, si manda un fail in cui si esclude questo canale dalla prossima chiamata
  // a dijkstra
  forwardChannelID = firstPathHop->channel;
  nextPeerID = firstPathHop->receiver;
  amountToForward = firstRouteHop->amountToForward;

  peer = hashTableGet(peers, event->peerID);

  forwardChannel = hashTableGet(channels, forwardChannelID);
  if(!isPresent(forwardChannel->ID, peer->channel)) {
    printf("Channel %ld has been closed\n", forwardChannel->ID);

    nextEvent = createEvent(eventIndex, simulatorTime, FINDROUTE, event->peerID, event->paymentID );
    events = heapInsert(events, nextEvent, compareEvent);
    return;
  }


  newBalance = forwardChannel->balance - amountToForward;
  if(newBalance < 0) {
    printf("not enough balance\n");

    payment->ignoredChannels = arrayInsert(payment->ignoredChannels, &(forwardChannel->ID));

    nextEvent = createEvent(eventIndex, simulatorTime, FINDROUTE, event->peerID, event->paymentID );
    events = heapInsert(events, nextEvent, compareEvent);

    return;
  }
  forwardChannel->balance = newBalance;

  //  printf("Peer %ld, balance %lf\n", event->peerID, forwardChannel->balance);


  //TODO: creare funzione generateForwardEvent che ha tutte le seguenti righe di codice fino alla fine


  eventType = nextPeerID == payment->receiver ? RECEIVEPAYMENT : FORWARDPAYMENT;
  /*  nextPeerPosition = getPeerPosition(nextPeerID, routeHops, isForward);
      if(nextPeerPosition == routeLen)
      eventType = RECEIVEPAYMENT;
      else if(nextPeerPosition>0 && nextPeerPosition<routeLen)
      eventType = FORWARDPAYMENT;
      else {
      printf("SendPayment %ld: wrong peer position %ld \n", event->paymentID, nextPeerPosition);
      return;
      }
  */
  simulatorTime += 0.1;
  nextEvent = createEvent(eventIndex, simulatorTime, eventType, nextPeerID, event->paymentID );

  events = heapInsert(events, nextEvent, compareEvent);

}

void forwardPayment(Event *event) {
  Payment* payment;
  Route* route;
  RouteHop* nextRouteHop, *previousRouteHop;
  Array* routeHops;
  long peerID, nextPeerID, prevPeerID, routeLen, nextChannelID;
  EventType eventType;
  Event* forwardEvent;
  double newBalance;
  Channel* prevChannel, *nextChannel;
  int isPolicyRespected, isSender;
  Peer* peer;

  printf("FORWARD PAYMENT %ld\n", event->paymentID);

  payment = hashTableGet(payments, event->paymentID);


  peer = hashTableGet(peers, event->peerID);

  route = payment->route;
  routeHops = route->routeHops;
  routeLen = arrayLen(routeHops);

  isSender = 1;
  nextRouteHop=getRouteHop(peer->ID, routeHops, isSender);
  isSender = 0;
  previousRouteHop = getRouteHop(peer->ID, routeHops, isSender);
  if(nextRouteHop == NULL || previousRouteHop == NULL) {
    printf("no route hop\n");
    return;
  }

  if(!isPresent(nextRouteHop->pathHop->channel, peer->channel)) {
    printf("Channel %ld has been closed\n", nextRouteHop->pathHop->channel);
    prevPeerID = previousRouteHop->pathHop->sender;
    eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    simulatorTime += 0.1;
    forwardEvent = createEvent(eventIndex, simulatorTime, eventType, prevPeerID, event->paymentID );
    events = heapInsert(events, forwardEvent, compareEvent);

    return;
  }

  if(!isCooperativeBeforeLockTest(event->peerID)){
    printf("Peer %ld is not cooperative before lock\n", event->peerID);
    payment->ignoredPeers = arrayInsert(payment->ignoredPeers, &(event->peerID));

    prevPeerID = previousRouteHop->pathHop->sender;
    eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    simulatorTime += 0.1;
    forwardEvent = createEvent(eventIndex, simulatorTime, eventType, prevPeerID, event->paymentID );
    events = heapInsert(events, forwardEvent, compareEvent);

    return;
  }

  /*

  if(!isCooperativeAfterLockTest(event->peerID)) {
    printf("Peer %ld is not cooperative after lock\n", event->peerID);

    prevChannel = hashTableGet(channels, previousRouteHop->pathHop->channel);
    closeChannel(prevChannel->channelInfoID);
    nextChannel = hashTableGet(channels, nextRouteHop->pathHop->channel);
    closeChannel(nextChannel->channelInfoID);

    prevPeerID = previousRouteHop->pathHop->sender;
    eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    simulatorTime += 0.1*previousRouteHop->timelock;
    forwardEvent = createEvent(eventIndex, simulatorTime, eventType, prevPeerID, event->paymentID );
    events = heapInsert(events, forwardEvent, compareEvent);
    return;
  }
  */


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
    simulatorTime += 0.1;
    forwardEvent = createEvent(eventIndex, simulatorTime, eventType, prevPeerID, event->paymentID );
    events = heapInsert(events, forwardEvent, compareEvent);


    return;
  }
  nextChannel->balance = newBalance;

  //  printf("Peer %ld, balance %lf\n", peerID, forwardChannel->balance);


  nextPeerID = nextRouteHop->pathHop->receiver;

  eventType = nextPeerID == payment->receiver ? RECEIVEPAYMENT : FORWARDPAYMENT;
  simulatorTime += 0.1;
  forwardEvent = createEvent(eventIndex, simulatorTime, eventType, nextPeerID, event->paymentID );
  events = heapInsert(events, forwardEvent, compareEvent);

}


void receivePayment(Event* event ) {
  long peerID, routeLen, prevPeerID;
  Route* route;
  Payment* payment;
  Array* routeHops;
  RouteHop* lastRouteHop;
  Channel* forwardChannel,*backwardChannel;
  Event* nextEvent;
  EventType eventType;

  printf("RECEIVE PAYMENT %ld\n", event->paymentID);
  peerID = event->peerID;

  payment = hashTableGet(payments, event->paymentID);
  route = payment->route;
  routeHops = route->routeHops;
  routeLen = arrayLen(routeHops);

  //TODO: non mi piace backwardChannel come nome
  lastRouteHop = arrayGet(routeHops, routeLen-1);
  forwardChannel = hashTableGet(channels, lastRouteHop->pathHop->channel);
  backwardChannel = hashTableGet(channels, forwardChannel->otherChannelDirectionID);

  backwardChannel->balance += lastRouteHop->amountToForward;

  payment->isSuccess = 1;
  //  printf("Peer %ld, balance %lf\n", peerID, backwardChannel->balance);

  prevPeerID = lastRouteHop->pathHop->sender;
  eventType = prevPeerID == payment->sender ? RECEIVESUCCESS : FORWARDSUCCESS;
  simulatorTime += 0.1;
  nextEvent = createEvent(eventIndex, simulatorTime, eventType, prevPeerID, event->paymentID);
  events = heapInsert(events, nextEvent, compareEvent);
}

//TODO: forse ha senso memorizzare nel peer tutti i payment ID che lo interessano (sia come sender che come receiver che come hop);
//     questo puo' servire sia per motivi statistici che anche per debugging: controllare cioe che il peer non riceva una richiesta
//     di settle o fail per un pagamento che non lo riguarda.

void forwardSuccess(Event* event) {
  RouteHop* prevHop, *nextHop;
  Payment* payment;
  int isSender;
  Channel* forwardChannel, * backwardChannel, *nextChannel, *prevChannel;
  long prevPeerID;
  Event* nextEvent;
  EventType eventType;
  Peer* peer;


  printf("FORWARD SUCCESS  %ld\n", event->paymentID);

  payment = hashTableGet(payments, event->paymentID);


  if(!isCooperativeAfterLockTest(event->peerID)) {
    printf("Peer %ld is not cooperative after lock\n", event->peerID);

    nextHop = getRouteHop(event->peerID, payment->route->routeHops, 1);
    prevHop = getRouteHop(event->peerID, payment->route->routeHops, 0);
    nextChannel = hashTableGet(channels, nextHop->pathHop->channel);
    prevChannel = hashTableGet(channels, prevHop->pathHop->channel);
    closeChannel(nextChannel->channelInfoID);
    closeChannel(prevChannel->channelInfoID);

    prevPeerID = prevHop->pathHop->sender;
    eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    simulatorTime += 0.1*prevHop->timelock;
    nextEvent = createEvent(eventIndex, simulatorTime, eventType, prevPeerID, event->paymentID );
    events = heapInsert(events, nextEvent, compareEvent);

    return;
  }

  isSender=0;
  prevHop = getRouteHop(event->peerID, payment->route->routeHops, isSender);


  forwardChannel = hashTableGet(channels, prevHop->pathHop->channel);
  backwardChannel = hashTableGet(channels, forwardChannel->otherChannelDirectionID);

  peer = hashTableGet(peers, event->peerID);

  if(!isPresent(backwardChannel->ID, peer->channel)) {
    prevPeerID = prevHop->pathHop->sender;
    eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    simulatorTime += 0.1;
    nextEvent = createEvent(eventIndex, simulatorTime, eventType, prevPeerID, event->paymentID);
    events = heapInsert(events, nextEvent, compareEvent);
    return;
  }

  backwardChannel->balance += prevHop->amountToForward;

 
  //  printf("Peer %ld, balance %lf\n", event->peerID, backwardChannel->balance);

  prevPeerID = prevHop->pathHop->sender;
  eventType = prevPeerID == payment->sender ? RECEIVESUCCESS : FORWARDSUCCESS;
  simulatorTime += 0.1;
  nextEvent = createEvent(eventIndex, simulatorTime, eventType, prevPeerID, event->paymentID);
  events = heapInsert(events, nextEvent, compareEvent);
}

void receiveSuccess(Event* event){

  printf("RECEIVE SUCCESS %ld\n", event->paymentID);

}


//TODO: uniformare tutti i vari RouteHop a next previous
void forwardFail(Event* event) {
  int next;
  Payment* payment;
  RouteHop* nextHop, *prevHop;
  Channel* nextChannel;
  long prevPeerID;
  Event* nextEvent;
  EventType eventType;
  Peer* peer;

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

  next=0;
  prevHop = getRouteHop(event->peerID, payment->route->routeHops, next);
  prevPeerID = prevHop->pathHop->sender;
  eventType = prevPeerID == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
  simulatorTime += 0.1;
  nextEvent = createEvent(eventIndex, simulatorTime, eventType, prevPeerID, event->paymentID);
  events = heapInsert(events, nextEvent, compareEvent);
}


void receiveFail(Event* event) {
  Payment* payment;
  RouteHop* firstHop;
  Channel* nextChannel;
  Event* nextEvent;
  Peer* peer;

  printf("RECEIVE FAIL %ld\n", event->paymentID);

  payment = hashTableGet(payments, event->paymentID);

  firstHop = arrayGet(payment->route->routeHops, 0);

  nextChannel = hashTableGet(channels, firstHop->pathHop->channel);
  peer = hashTableGet(peers, event->peerID);
  if(isPresent(nextChannel->ID, peer->channel))
    nextChannel->balance += firstHop->amountToForward;

  if(payment->isSuccess == 1 ) return; //it means that money actually arrived to the destination but a peer was not cooperative when forwarding the success

  nextEvent = createEvent(eventIndex, simulatorTime, FINDROUTE, payment->sender, payment->ID);
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
