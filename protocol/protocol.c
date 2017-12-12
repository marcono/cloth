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

Peer* createPeer(long ID, long channelsSize) {
  Peer* peer;

  peer = GC_MALLOC(sizeof(Peer));
  peer->ID=ID;
  peer->channelsSize = channelsSize;
  peer->channel = arrayInitialize(peer->channelsSize);

  return peer;
}

ChannelInfo* createChannelInfo(long ID, long peer1, long peer2, double capacity) {
  ChannelInfo* channelInfo;

  channelInfo = GC_MALLOC(sizeof(ChannelInfo));
  channelInfo->ID = ID;
  channelInfo->peer1 = peer1;
  channelInfo->peer2 = peer2;
  channelInfo->capacity = capacity;

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

  channelInfo = createChannelInfo(channelInfoID, peer1->ID, peer2->ID, 5.0);
  hashTablePut(channelInfos, channelInfo->ID, channelInfo);
  channelInfoID++;

  //TODO: meglio mettere il settaggio della balance del canale (sempre pari a capacity/2) dentro
  //la funzione create channel, magari passando come parametro direttamente l'oggetto channelInfos (cosi si prende ID e capacity)
  channel = createChannel(channelID, channelInfo->ID, peer2->ID, policy, channelInfo->capacity*0.5);
  hashTablePut(channels, channel->ID, channel);
  peer1->channel = arrayInsert(peer1->channel, &(channel->ID));
  channelID++;

  channel = createChannel(channelID, channelInfo->ID, peer1->ID, policy, channelInfo->capacity*0.5 );
  hashTablePut(channels, channel->ID, channel);
  peer2->channel =arrayInsert(peer2->channel, &(channel->ID)); 
  channelID++;

}



//TODO: forse e' il caso di fare un evento findroute  che trova la route e poi ad ogni funzione
// sendpayment forwardpayment etc. si passa routeHop anziche intera route
void sendPayment(long paymentID) {
  Payment* payment;
  long receiver, sender, nextPeerHop, forwardChannel;
  Array* pathHops;
  double amountToSend, amountToForward, channelBalance, newBalance;
  int finalTimelock=9;
  Route* route;
  PathHop* firstPathHop;
  RouteHop* firstRouteHop;
  Channel* channel;
  Event* event;

  payment = hashTableGet(payments, paymentID);
  receiver = payment->receiver;
  sender = payment->sender;
  amountToSend =  payment->amount;


  pathHops = dijkstra(sender, receiver, amountToSend, NULL, NULL);
  if(pathHops==NULL) {
    printf("SendPayment %ld: No available path\n", paymentID);
    return;
  }

  route = transformPathIntoRoute(pathHops, amountToSend, finalTimelock);
  if(route==NULL) {
    printf("SendPayment %ld: No availabe route\n", paymentID);
    return;
  }

  firstRouteHop = arrayGet(route->routeHops, 0);
  firstPathHop = firstRouteHop->pathHop;
  forwardChannel = firstPathHop->channel;
  nextPeerHop = firstPathHop->receiver;
  amountToForward = firstRouteHop->amountToForward;

  channel = hashTableGet(channels, forwardChannel);
  channelBalance = channel->balance;

  newBalance = channelBalance - amountToForward;
  if(newBalance < 0) {
    printf("SendPayment %ld: Not enough balance\n", paymentID);
    return;
  }

  channel->balance = newBalance;

  payment->route = route;
  simulatorTime += 0.1;
  event = createEvent(eventID, simulatorTime, SEND, nextPeerHop, paymentID );

  events = heapInsert(events, event, compareEvent);

  printf("Everything's gonna be alright\n");

  return;
}

void forwardPayment(long paymentID) {
  Payment* payment;

  payment = hashTableGet(payments, paymentID);

  printf("HEY/n");

}


void receivePayment(long paymentID) {
    Payment* payment;

    payment = hashTableGet(payments, paymentID);

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
