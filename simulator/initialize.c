#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../gc-7.2/include/gc.h"
#include "../utils/hashTable.h"
#include "../utils/array.h"
#include "../protocol/protocol.h"
#include "initialize.h"
#include "event.h"
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <stdint.h>

#define MAXMICRO 1E3
#define MINMICRO 1
#define MAXSMALL 1E8
#define MINSMALL 1E3
#define MAXMEDIUM 1E11
#define MINMEDIUM 1E8

long eventIndex;
Heap* events;
uint64_t simulatorTime;


void initializeEvents(long nPayments, double paymentMean) {
  long i, senderID, receiverID;
  uint64_t  paymentAmount=0, eventTime=0 ;
  uint32_t nextEventInterval;
  unsigned int paymentClass;
  double paymentClassP[]= {0.7, 0.2, 0.1};
  gsl_ran_discrete_t* discrete;
  Payment *payment;
  Event* event;

  discrete = gsl_ran_discrete_preproc(3, paymentClassP);

  events = heapInitialize(nPayments);

  for(i=0;i<nPayments;i++) {

    do{
      senderID = gsl_rng_uniform_int(r,peerIndex);
      receiverID = gsl_rng_uniform_int(r, peerIndex);
    } while(senderID==receiverID);


    //TODO: forse vanno fatte piu classi di pagamento con intervalli piu piccoli,
    // perche passa da pagamenti dell'ordine di 100 (classe 0) a pagamenti dell'ordine di 1.000.000 (classe 1)
    //FIXME: metti ampiezza giusta dell'intervallo, aumentando il numero di classi per non avere intervalli troppo ampi
    paymentClass = gsl_ran_discrete(r, discrete);
    printf("%d", paymentClass);
    switch(paymentClass) {
    case 0:
      paymentAmount = gsl_rng_uniform_int(r, 1000) + 1;
      break;
    case 1:
      paymentAmount = (gsl_rng_uniform_int(r, 10000) + 1)*1000;
      break;
    case 2:
      paymentAmount = (gsl_rng_uniform_int(r, 1000) + 1)*1E8;
      break;
    }
    printf(" %ld\n", paymentAmount);

    payment = createPayment(paymentIndex, senderID, receiverID, paymentAmount);
    hashTablePut(payments, payment->ID, payment);

    nextEventInterval = 1000*gsl_ran_exponential(r, paymentMean);
    eventTime += nextEventInterval;
    event = createEvent(eventIndex, eventTime, FINDROUTE, senderID, payment->ID);
    events = heapInsert(events, event, compareEvent);
  }

}

void initializeSimulatorData(long nPayments, double paymentMean ) {
  eventIndex = 0;
  simulatorTime = 1;
  initializeEvents(nPayments, paymentMean);
}



/*
void initialize() {
  long i;
  Peer* peer;
  Channel* channel;
  ChannelInfo* channelInfo;
  

  gsl_rng *r;
  const gsl_rng_type * T;

  gsl_rng_env_setup();

  T = gsl_rng_default;
  r = gsl_rng_alloc (T);

  nPeers=5;
  nChannels=2;



  peers = hashTableInitialize(2);
  channels = hashTableInitialize(2);
  channelInfos= hashTableInitialize(2);


  for(i=0; i<nPeers; i++) {
    peer = createPeer(nChannels);
    hashTablePut(peers, peer->ID, peer);
  }


  long j, counterpartyID;
  Peer* counterparty;
  Policy policy;
  policy.feeBase=0.1;
  policy.timelock=5;
  policy.feeProportional=0.0;

  srand(time(NULL));
  for(i=0; i<nPeers; i++) {
    peer = hashTableGet(peers, i);
    for(j=0; j<nChannels && (arrayLen(peer->channel) < nChannels); j++){


    do {
          counterpartyID = gsl_rng_uniform_int(r,nPeers);
      }while(counterpartyID==peer->ID);


      counterparty = hashTableGet(peers, counterpartyID);
      if(arrayLen(counterparty->channel)>=nChannels) continue;

      channelInfo=createChannelInfo(peer->ID, counterparty->ID, 1.0);
      hashTablePut(channelInfos, channelInfo->ID,channelInfo);

      channel=createChannel(channelInfo->ID, counterparty->ID, policy);
      hashTablePut(channels, channel->ID, channel);
      peer->channel=arrayInsert(peer->channel, &(channel->ID));

      channel = createChannel(channelInfo->ID, peer->ID, policy);
      hashTablePut(channels, channel->ID, channel);
      counterparty->channel=arrayInsert(counterparty->channel, &(channel->ID));

    } 
  }

  long *currChannelID;
  for(i=0; i<nPeers; i++) {
    peer = hashTableGet(peers, i);
    for(j=0; j<nChannels; j++) {
      currChannelID=arrayGet(peer->channel, j);
      if(currChannelID==NULL) continue;
      channel = hashTableGet(channels, *currChannelID);
      printf("Peer %ld is connected to peer %ld through channel %ld\n", i, channel->counterparty, channel->ID);
    } 
  }

}
*/
