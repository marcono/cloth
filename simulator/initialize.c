#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../gc-7.2/include/gc.h"
#include "../utils/hashTable.h"
#include "../utils/array.h"
#include "../protocol/protocol.h"
#include "initialize.h"
#include <gsl/gsl_rng.h>

HashTable* peers, *channels, *channelInfos;
long nPeers, nChannels;

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

