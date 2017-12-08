#include <stdlib.h>
#include "../gc-7.2/include/gc.h"
#include "protocol.h"
#include "../utils/array.h"
#include "../utils/hashTable.h"
#include "../simulator/initialize.h"

long peerID=0, channelInfoID=0, channelID=0;

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

void connectPeers(long peerID1, long peerID2) {
  Peer* peer1, *peer2;
  Policy policy;
  Channel* channel;
  ChannelInfo *channelInfo;

  policy.feeBase = 1.0;
  policy.feeProportional = 0.0;
  policy.timelock = 5;

  peer1 = hashTableGet(peers, peerID1);
  peer2 = hashTableGet(peers, peerID2);

  channelInfo = createChannelInfo(peer1->ID, peer2->ID, 5.0);
  hashTablePut(channelInfos, channelInfo->ID, channelInfo);

  channel = createChannel(channelInfo->ID, peer2->ID, policy);
  hashTablePut(channels, channel->ID, channel);
  peer1->channel = arrayInsert(peer1->channel, &(channel->ID));

  channel = createChannel(channelInfo->ID, peer1->ID, policy);
  hashTablePut(channels, channel->ID, channel);
  peer2->channel =arrayInsert(peer2->channel, &(channel->ID)); 


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
