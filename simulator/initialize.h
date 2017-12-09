#ifndef INITIALIZE_H
#define INITIALIZE_H
#include "../utils/hashTable.h"

extern long nPeers;
extern long nChannels;
extern HashTable* peers;
extern HashTable* channels;
extern HashTable* channelInfos;
extern HashTable* payments;

void initialize();


#endif
