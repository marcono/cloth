#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../gc-7.2/include/gc.h"
#include "../utils/hashTable.h"
#include "../utils/array.h"
#include "../protocol/protocol.h"

void initialize() {
  HashTable* peers, *channels, *channelInfos;
  long i, channelsSize=2, peersNumber=10;
  Peer* peer;
  Channel* channel;
  ChannelInfo* channelInfo;

  initializeProtocol();

  peers = hashTableInitialize(2);
  channels = hashTableInitialize(2);
  channelInfos= hashTableInitialize(2);


  for(i=0; i<peersNumber; i++) {
    peer = createPeer(channelsSize);
    hashTablePut(peers, peer->ID, peer);
  }


  long j, counterpartyID;
  Peer* counterparty;
  Policy policy;
  policy.fee=0.0;
  policy.timelock=0.0;
  srand(time(NULL));
  for(i=0; i<peersNumber; i++) {
    peer = hashTableGet(peers, i);
    for(j=0; j<channelsSize && (arrayGetNElems(peer->channel) < channelsSize); j++){

      do {
        counterpartyID = rand()%peersNumber;
      }while(counterpartyID==peer->ID);

      counterparty = hashTableGet(peers, counterpartyID);
      if(arrayGetNElems(counterparty->channel)>=channelsSize) continue;

      channelInfo=createChannelInfo(peer->ID, counterparty->ID, 0.0);
      hashTablePut(channelInfos, channelInfo->ID,channelInfo);

      channel=createChannel(channelInfo->ID, counterparty->ID, policy);
      hashTablePut(channels, channel->ID, channel);
      arrayInsert(peer->channel, channel->ID);

      channel = createChannel(channelInfo->ID, peer->ID, policy);
      hashTablePut(channels, channel->ID, channel);
      arrayInsert(counterparty->channel, channel->ID);
      
    } 
  }

  long currChannelID;
  for(i=0; i<peersNumber; i++) {
    peer = hashTableGet(peers, i);
    for(j=0; j<channelsSize; j++) {
      currChannelID=arrayGet(peer->channel, j);
      if(currChannelID==-1) continue;
      channel = hashTableGet(channels, currChannelID);
      printf("Peer %ld is connected to peer %ld through channel %ld\n", i, channel->counterparty, channel->channelInfoID);
    } 
  }

}

