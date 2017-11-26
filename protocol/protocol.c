#include <stdlib.h>
#include "../gc-7.2/include/gc.h"
#include "protocol.h"
#include "../utils/array.h"

Peer* createPeer(long channelsSize) {
  Peer* peer;

  peer = GC_MALLOC(sizeof(Peer));
  peer->ID=peerID;
  peerID++;
  peer->channelsSize = channelsSize;
  peer->channel = arrayInitialize(peer->channelsSize);

  return peer;
}

ChannelInfo* createChannelInfo(long peer1, long peer2, double capacity) {
  ChannelInfo* channelInfo;

  channelInfo = GC_MALLOC(sizeof(ChannelInfo));
  channelInfo->ID = channelInfoID;
  channelInfoID++;
  channelInfo->peer1 = peer1;
  channelInfo->peer2 = peer2;
  channelInfo->capacity = capacity;

  return channelInfo;
}

Channel* createChannel(long channelInfoID, long counterparty, Policy policy){
  Channel* channel;

  channel = GC_MALLOC(sizeof(Channel));
  channel->ID = channelID;
  channelID++;
  channel->channelInfoID = channelInfoID;
  channel->counterparty = counterparty;
  channel->policy = policy;

  return channel;
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
