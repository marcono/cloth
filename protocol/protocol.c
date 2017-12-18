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

  paymentIndex++;

  return p;
}

void connectPeers(long peerID1, long peerID2) {
  Peer* peer1, *peer2;
  Policy policy;
  Channel* channel;
  ChannelInfo *channelInfo;

  policy.feeBase = 0.1;
  policy.feeProportional = 0.0;
  policy.timelock = 1;

  peer1 = hashTableGet(peers, peerID1);
  peer2 = hashTableGet(peers, peerID2);

  channelInfo = createChannelInfo(channelInfoIndex, peer1->ID, peer2->ID, 5.0);
  hashTablePut(channelInfos, channelInfo->ID, channelInfo);


  //TODO: meglio mettere il settaggio della balance del canale (sempre pari a capacity/2) dentro
  //la funzione create channel, magari passando come parametro direttamente l'oggetto channelInfos (cosi si prende ID e capacity)
  channel = createChannel(channelIndex, channelInfo->ID, peer2->ID, policy, channelInfo->capacity*0.5);
  hashTablePut(channels, channel->ID, channel);
  peer1->channel = arrayInsert(peer1->channel, &(channel->ID));


  channel = createChannel(channelIndex, channelInfo->ID, peer1->ID, policy, channelInfo->capacity*0.5 );
  hashTablePut(channels, channel->ID, channel);
  peer2->channel =arrayInsert(peer2->channel, &(channel->ID)); 


}

void findRoute(Event* event) {
  Payment *payment;
  long sender, receiver;
  double amountToSend;
  Array* pathHops;
  Route* route;
  int finalTimelock=9;
  PathHop* firstHop;
  Event* sendEvent;

  payment = hashTableGet(payments, event->paymentID);
  receiver = payment->receiver;
  sender = payment->sender;
  amountToSend =  payment->amount;


  pathHops = dijkstra(sender, receiver, amountToSend, NULL, NULL);
  if(pathHops==NULL) {
    printf("SendPayment %ld: No available path\n", event->paymentID);
    return;
  }

  route = transformPathIntoRoute(pathHops, amountToSend, finalTimelock);
  if(route==NULL) {
    printf("SendPayment %ld: No available route\n", event->paymentID);
    return;
  }

  payment->route = route;

  firstHop = arrayGet(pathHops, 0);

  sendEvent = createEvent(eventIndex, simulatorTime, SENDPAYMENT, firstHop->sender, event->paymentID );
  events = heapInsert(events, sendEvent, compareEvent);

}

RouteHop* getRouteHop(long peerID, Array* routeHops, int isForward) {
  RouteHop* routeHop;
  long i, index = -1;

  for(i=0; i<arrayLen(routeHops); i++) {
    routeHop = arrayGet(routeHops, i);

    if(isForward && routeHop->pathHop->sender == peerID)
      index = i;

    if(!isForward && routeHop->pathHop->receiver == peerID)
      index = i;
  }

  if(index == -1) return NULL;

  return arrayGet(routeHops, index);
}



void sendPayment(Event* event) {
  Payment* payment;
  long  nextPeerID, forwardChannelID, nextPeerPosition, routeLen;
  double amountToForward, newBalance;
  Route* route;
  PathHop* firstPathHop;
  RouteHop* firstRouteHop;
  Array* routeHops;
  Channel* forwardChannel;
  Event* forwardEvent;
  int isForward=1; //TODO: fare enum di tipo bool?
  EventType eventType;

  printf("send payment\n");

  payment = hashTableGet(payments, event->paymentID);
  route = payment->route;
  routeHops = route->routeHops;
  routeLen = arrayLen(routeHops);

  firstRouteHop = arrayGet(routeHops, 0);
  firstPathHop = firstRouteHop->pathHop;
  forwardChannelID = firstPathHop->channel;
  nextPeerID = firstPathHop->receiver;
  amountToForward = firstRouteHop->amountToForward;

  forwardChannel = hashTableGet(channels, forwardChannelID);

  newBalance = forwardChannel->balance - amountToForward;
  if(newBalance < 0) {
    printf("SendPayment %ld: Not enough balance\n", event->paymentID);
    return;
  }
  forwardChannel->balance = newBalance;

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
  forwardEvent = createEvent(eventIndex, simulatorTime, eventType, nextPeerID, event->paymentID );

  events = heapInsert(events, forwardEvent, compareEvent);

}

void forwardPayment(Event *event) {
  Payment* payment;
  Route* route;
  RouteHop* routeHop;
  Array* routeHops;
  long peerID, nextPeerID, routeLen, nextPeerPosition;
  int isForward;
  EventType eventType;
  Event* forwardEvent;

  printf("forward payment\n");

  peerID = event->peerID;

  payment = hashTableGet(payments, event->paymentID);
  route = payment->route;
  routeHops = route->routeHops;
  routeLen = arrayLen(routeHops);

  isForward = 1;
  routeHop=getRouteHop(peerID, routeHops, isForward);
  if(routeHop == NULL) {
    printf("ForwardPayment %ld: no route hop\n", event->paymentID);
    return;
  }

  nextPeerID = routeHop->pathHop->receiver;

  eventType = nextPeerID == payment->receiver ? RECEIVEPAYMENT : FORWARDPAYMENT;
  simulatorTime += 0.1;
  forwardEvent = createEvent(eventIndex, simulatorTime, eventType, nextPeerID, event->paymentID );
  events = heapInsert(events, forwardEvent, compareEvent);

}


void receivePayment(Event* event ) {


  printf("receive payment\n");

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
