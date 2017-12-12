#ifndef INITIALIZE_H
#define INITIALIZE_H
#include "../utils/hashTable.h"
#include "../utils/heap.h"

extern long nPeers;
extern long nChannels;
extern HashTable* peers;
extern HashTable* channels;
extern HashTable* channelInfos;
extern HashTable* payments;
//TODO: mettere time in un altro file (globalSimulator.h per esempio)
extern double simulatorTime;
extern Heap*  events;

void initialize();


#endif
