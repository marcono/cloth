#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
//#include <gsl/gsl_rng.h>
//#include <gsl/gsl_randist.h>
#include "json.h"
#include "gsl_rng.h"
#include "gsl_cdf.h"

#include "./simulator/event.h"
#include "./simulator/initialize.h"
#include "./utils/heap.h"
#include "./utils/hashTable.h"
//#include "./gc-7.2/include/gc.h"
#include "./utils/array.h"
#include "./protocol/findRoute.h"
#include "./protocol/protocol.h"
#include "./simulator/stats.h"
#include "./global.h"
#include "./utils/list.h"

/*void printBalances() {
  long i, j, *channelID;
  Peer* peer;
  Array* peerChannels;
  Channel* channel;

  printf("PRINT BALANCES\n");
  for(i=0; i<peerIndex; i++) {
    peer = arrayGet(peers, i);
    peerChannels = peer->channel;
    for(j=0; j<arrayLen(peerChannels); j++) {
      channelID = arrayGet(peerChannels, j);
      channel = arrayGet(channels, *channelID);
      printf("Peer %ld, Channel %ld, Balance %lf\n", peer->ID, channel->channelInfoID, channel->balance);
    }
  }
  }

void printPayments() {
  long i, j;
  Payment *payment;
  Array* routeHops;
  RouteHop* hop;
  Peer* peer;
  Channel* forward, *backward;

  for(i = 0; i < paymentIndex; i++) {
    payment = arrayGet(payments, i);
    printf("PAYMENT %ld\n", payment->ID);
    if(payment->route==NULL) continue;
    routeHops = payment->route->routeHops;
    for(j=0; j<arrayLen(routeHops); j++){
      hop = arrayGet(routeHops, j);
      peer = arrayGet(peers, hop->pathHop->sender);
      if(isPresent(hop->pathHop->channel, peer->channel)) {
        forward = arrayGet(channels, hop->pathHop->channel);
        backward = arrayGet(channels, forward->otherChannelDirectionID);
        printf("Sender %ld, Receiver %ld, Channel %ld, Balance forward %ld, Balance backward %ld\n",
               hop->pathHop->sender, hop->pathHop->receiver, forward->channelInfoID, forward->balance, backward->balance);

      }
      else {
        forward = arrayGet(channels, hop->pathHop->channel);
        printf("Sender %ld, Receiver %ld, Channel %ld, Channel closed\n", hop->pathHop->sender, hop->pathHop->receiver, forward->channelInfoID);
      }
    }
  }
}

struct json_object* jsonNewChannelDirection(Channel* direction) {
  struct json_object* jdirection;
  struct json_object* jID, *jcounterpartyID, *jbalance, *jpolicy, *jfeebase, *jfeeprop, *jtimelock;

  jdirection = json_object_new_object();

  jID = json_object_new_int64(direction->ID);
  jcounterpartyID = json_object_new_int64(direction->counterparty);
  jbalance = json_object_new_int64(direction->balance);

  jpolicy = json_object_new_object();
  jfeebase = json_object_new_int64(direction->policy.feeBase);
  jfeeprop = json_object_new_int64(direction->policy.feeProportional);
  jtimelock = json_object_new_int(direction->policy.timelock);
  json_object_object_add(jpolicy, "FeeBase", jfeebase );
  json_object_object_add(jpolicy, "FeeProportional", jfeeprop);
  json_object_object_add(jpolicy, "Timelock", jtimelock );
 
  json_object_object_add(jdirection, "ID", jID );
  json_object_object_add(jdirection, "CounterpartyID", jcounterpartyID);
  json_object_object_add(jdirection, "Balance", jbalance );
  json_object_object_add(jdirection, "Policy", jpolicy );

  return jdirection;
}

struct json_object* jsonNewChannel(ChannelInfo *channel) {
  struct json_object* jchannel;
  struct json_object* jID, *jcapacity, *jlatency, *jpeer1, *jpeer2, *jdirection1, *jdirection2;

  jchannel = json_object_new_object();

  jID = json_object_new_int64(channel->ID);
  jpeer1 = json_object_new_int64(channel->peer1);
  jpeer2 = json_object_new_int64(channel->peer2);
  jcapacity = json_object_new_int64(channel->capacity);
  jlatency = json_object_new_int(channel->latency);
  jdirection1 = jsonNewChannelDirection(arrayGet(channels, channel->channelDirection1));
  jdirection2 = jsonNewChannelDirection(arrayGet(channels, channel->channelDirection2));

  json_object_object_add(jchannel, "ID", jID);
  json_object_object_add(jchannel, "Peer1ID", jpeer1);
  json_object_object_add(jchannel, "Peer2ID", jpeer2);
  json_object_object_add(jchannel, "Capacity", jcapacity);
  json_object_object_add(jchannel, "Latency", jlatency);
  json_object_object_add(jchannel, "Direction1", jdirection1);
  json_object_object_add(jchannel, "Direction2", jdirection2);

  return jchannel;
}

struct json_object* jsonNewPeer(Peer *peer) {
  struct json_object* jpeer;
  struct json_object *jID, *jChannelSize, *jChannelIDs, *jChannelID, *jwithholdsR;
  long i, *channelID;

  jpeer = json_object_new_object();

  jID = json_object_new_int64(peer->ID);
  jChannelSize = json_object_new_int64(arrayLen(peer->channel));
  jwithholdsR = json_object_new_int(peer->withholdsR);

  jChannelIDs = json_object_new_array();
  for(i=0; i<arrayLen(peer->channel); i++) {
    channelID = arrayGet(peer->channel, i);
    jChannelID = json_object_new_int64(*channelID);
    json_object_array_add(jChannelIDs, jChannelID);
  }

  json_object_object_add(jpeer, "ID", jID);
  json_object_object_add(jpeer,"ChannelsSize",jChannelSize);
  json_object_object_add(jpeer, "WithholdsR", jwithholdsR);
  json_object_object_add(jpeer, "ChannelIDs", jChannelIDs);

  return jpeer;
}

void jsonWriteInput() {
  long i;
  struct json_object* jpeer, *jpeers, *jchannel, *jchannels;
  struct json_object* jobj;
  Peer* peer;
  ChannelInfo* channel;

  jobj = json_object_new_object();
  jpeers = json_object_new_array();
  jchannels = json_object_new_array();

  for(i=0; i<peerIndex; i++) {
    peer = arrayGet(peers, i);
    jpeer = jsonNewPeer(peer);
    json_object_array_add(jpeers, jpeer);
  }
  json_object_object_add(jobj, "Peers",jpeers);

  for(i=0; i<channelInfoIndex; i++){
    channel = arrayGet(channelInfos, i);
    jchannel = jsonNewChannel(channel);
    json_object_array_add(jchannels, jchannel);
  }
  json_object_object_add(jobj, "Channels", jchannels);

  //  printf("%s\n", json_object_to_json_string(jobj));

  json_object_to_file_ext("simulator_input.json", jobj, JSON_C_TO_STRING_PRETTY);

}

void csvWriteInput() {
  FILE *csvPeer, *csvChannelInfo, *csvChannel;
  Peer* peer;
  ChannelInfo* channelInfo;
  Channel* channel;
  long i, j, *channelID;

  csvPeer = fopen("peer.csv", "w");
   if(csvPeer==NULL) {
    printf("ERROR cannot open file peer.csv\n");
    return;
  }

  fprintf(csvPeer, "ID,ChannelsSize,WithholdsR,ChannelIDs\n");
  for(i=0; i<peerIndex; i++) {
    peer = arrayGet(peers, i);
    fprintf(csvPeer, "%ld,%ld,%d,", peer->ID, arrayLen(peer->channel), peer->withholdsR);
    for(j=0; j<arrayLen(peer->channel); j++) {
      channelID = arrayGet(peer->channel, j);
      if(j==arrayLen(peer->channel)-1) 
        fprintf(csvPeer,"%ld\n", *channelID);
      else
        fprintf(csvPeer,"%ld-", *channelID);
    }
  }


  fclose(csvPeer);
 
  csvChannelInfo = fopen("channelInfo.csv", "w");
  if(csvChannelInfo==NULL) {
    printf("ERROR cannot open file channelInfo.csv\n");
    return;
  }

  fprintf(csvChannelInfo, "ID,Peer1,Peer2,Direction1,Direction2,Capacity,Latency\n");
  for(i=0; i<channelInfoIndex; i++) {
    channelInfo = arrayGet(channelInfos, i);
    fprintf(csvChannelInfo,"%ld,%ld,%ld,%ld,%ld,%ld,%d\n",channelInfo->ID, channelInfo->peer1, channelInfo->peer2, channelInfo->channelDirection1, channelInfo->channelDirection2, channelInfo->capacity, channelInfo->latency);
  }

  fclose(csvChannelInfo);

  csvChannel = fopen("channel.csv", "w");
  if(csvChannel==NULL) {
    printf("ERROR cannot open file channel.csv\n");
    return;
  }


  fprintf(csvChannel, "ID,ChannelInfo,OtherDirection,Counterparty,Balance,FeeBase,FeeProportional,Timelock\n");
  for(i=0; i<channelIndex; i++) {
    channel = arrayGet(channels, i);
    fprintf(csvChannel, "%ld,%ld,%ld,%ld,%ld,%d,%d,%d\n", channel->ID, channel->channelInfoID, channel->otherChannelDirectionID, channel->counterparty, channel->balance, channel->policy.feeBase, channel->policy.feeProportional, channel->policy.timelock);
  }


  fclose(csvChannel);

}

*/

void csvWriteOutput() {
  FILE* csvChannelInfoOutput, *csvChannelOutput, *csvPaymentOutput, *csvPeerOutput;
  long i,j, *id;
  ChannelInfo* channelInfo;
  Channel* channel;
  Payment* payment;
  Peer* peer;
  Route* route;
  Array* hops;
  RouteHop* hop;

  csvChannelInfoOutput = fopen("channelInfo_output.csv", "w");
  if(csvChannelInfoOutput  == NULL) {
    printf("ERROR cannot open channelInfo_output.csv\n");
    return;
  }
  fprintf(csvChannelInfoOutput, "ID,Direction1,Direction2,Peer1,Peer2,Capacity,Latency,IsClosed\n");

  for(i=0; i<channelInfoIndex; i++) {
    channelInfo = arrayGet(channelInfos, i);
    fprintf(csvChannelInfoOutput, "%ld,%ld,%ld,%ld,%ld,%ld,%d,%d\n", channelInfo->ID, channelInfo->channelDirection1, channelInfo->channelDirection2, channelInfo->peer1, channelInfo->peer2, channelInfo->capacity, channelInfo->latency, channelInfo->isClosed);
  }

  fclose(csvChannelInfoOutput);

  csvChannelOutput = fopen("channel_output.csv", "w");
  if(csvChannelInfoOutput  == NULL) {
    printf("ERROR cannot open channel_output.csv\n");
    return;
  }
  fprintf(csvChannelOutput, "ID,ChannelInfo,OtherDirection,Counterparty,Balance,FeeBase,FeeProportional,MinHTLC,Timelock,isClosed\n");

  for(i=0; i<channelIndex; i++) {
    channel = arrayGet(channels, i);
    fprintf(csvChannelOutput, "%ld,%ld,%ld,%ld,%ld,%d,%d,%d,%d,%d\n", channel->ID, channel->channelInfoID, channel->otherChannelDirectionID, channel->counterparty, channel->balance, channel->policy.feeBase, channel->policy.feeProportional, channel->policy.minHTLC, channel->policy.timelock, channel->isClosed);
  }

  fclose(csvChannelOutput);

  csvPaymentOutput = fopen("payment_output.csv", "w");
  if(csvChannelInfoOutput  == NULL) {
    printf("ERROR cannot open payment_output.csv\n");
    return;
  }
  fprintf(csvPaymentOutput, "ID,Sender,Receiver,Amount,Time,EndTime,IsSuccess,UncoopAfter,UncoopBefore,IsTimeout,Attempts,Route\n");

  for(i=0; i<paymentIndex; i++)  {
    payment = arrayGet(payments, i);
    fprintf(csvPaymentOutput, "%ld,%ld,%ld,%ld,%ld,%ld,%d,%d,%d,%d,%d,", payment->ID, payment->sender, payment->receiver, payment->amount, payment->startTime, payment->endTime, payment->isSuccess, payment->uncoopAfter, payment->uncoopBefore, payment->isTimeout, payment->attempts);
    route = payment->route;

    if(route==NULL)
      fprintf(csvPaymentOutput, "-1");
    else {
      hops = route->routeHops;
      for(j=0; j<arrayLen(hops); j++) {
        hop = arrayGet(hops, j);
        if(j==arrayLen(hops)-1)
          fprintf(csvPaymentOutput,"%ld",hop->pathHop->channel);
        else
          fprintf(csvPaymentOutput,"%ld-",hop->pathHop->channel);
      }
    }
    fprintf(csvPaymentOutput,"\n");
  }

  fclose(csvPaymentOutput);


  csvPeerOutput = fopen("peer_output.csv", "w");
  if(csvChannelInfoOutput  == NULL) {
    printf("ERROR cannot open peer_output.csv\n");
    return;
  }
  fprintf(csvPeerOutput, "ID,OpenChannels,IgnoredPeers,IgnoredChannels\n");

  for(i=0; i<peerIndex; i++) {
    peer = arrayGet(peers, i);

    fprintf(csvPeerOutput, "%ld,", peer->ID);

    if(arrayLen(peer->channel)==0)
      fprintf(csvPeerOutput, "-1");
    else {
      for(j=0; j<arrayLen(peer->channel); j++) {
        id = arrayGet(peer->channel, j);
        if(j==arrayLen(peer->channel)-1)
          fprintf(csvPeerOutput,"%ld",*id);
        else
          fprintf(csvPeerOutput,"%ld-",*id);
      }
    }
    fprintf(csvPeerOutput,",");

    if(arrayLen(peer->ignoredPeers)==0)
      fprintf(csvPeerOutput, "-1");
    else {
      for(j=0; j<arrayLen(peer->ignoredPeers); j++) {
        id = arrayGet(peer->ignoredPeers, j);
        if(j==arrayLen(peer->ignoredPeers)-1)
          fprintf(csvPeerOutput,"%ld",*id);
        else
          fprintf(csvPeerOutput,"%ld-",*id);
      }
    }
    fprintf(csvPeerOutput,",");

    if(arrayLen(peer->ignoredChannels)==0)
      fprintf(csvPeerOutput, "-1");
    else {
      for(j=0; j<arrayLen(peer->ignoredChannels); j++) {
        id = arrayGet(peer->ignoredChannels, j);
        if(j==arrayLen(peer->ignoredChannels)-1)
          fprintf(csvPeerOutput,"%ld",*id);
        else
          fprintf(csvPeerOutput,"%ld-",*id);
      }
    }
    fprintf(csvPeerOutput,"\n");
  
  }

  fclose(csvPeerOutput);

}

pthread_mutex_t pathsMutex;
pthread_mutex_t peersMutex;
pthread_mutex_t jobsMutex;
Array** paths;
Node* jobs=NULL;


void initializeThreads() {
  long i;
  Payment *payment;
  pthread_t tid[PARALLEL];
  int dataIndex[PARALLEL];

  pthread_mutex_init(&peersMutex, NULL);
  pthread_mutex_init(&jobsMutex, NULL);
  pthread_mutex_init(&pathsMutex, NULL);

  paths = malloc(sizeof(Array*)*paymentIndex);

  for(i=0; i<paymentIndex ;i++){
    paths[i] = NULL;
    payment = arrayGet(payments, i);
    jobs = push(jobs, payment->ID);
  }

  for(i=0; i<PARALLEL; i++) {
    dataIndex[i] = i;
    pthread_create(&(tid[i]), NULL, &dijkstraThread, &(dataIndex[i]));
  }

  for(i=0; i<PARALLEL; i++)
    pthread_join(tid[i], NULL);


}

uint64_t readPreInputAndInitialize(int isPreprocTopology) {
  long nPayments, nPeers, nChannels, capacityPerChannel;
  double paymentMean, pUncoopBefore, pUncoopAfter, RWithholding, sameDest;
  struct json_object* jpreinput, *jobj;
  unsigned int isPreprocPayments=1;
  int gini, sigmaTopology;
  char answer;
  clock_t  begin, end;
  double timeSpent=0.0;
  struct timespec start, finish;
  uint64_t endTime;
  double sigmaAmount;


  jpreinput = json_object_from_file("preinput.json");

  jobj = json_object_object_get(jpreinput, "PaymentMean");
  paymentMean = json_object_get_double(jobj);
  jobj = json_object_object_get(jpreinput, "NPayments");
  nPayments = json_object_get_int64(jobj);
  jobj = json_object_object_get(jpreinput, "NPeers");
  nPeers = json_object_get_int64(jobj);
  jobj = json_object_object_get(jpreinput, "NChannels");
  nChannels = json_object_get_int64(jobj);
  jobj = json_object_object_get(jpreinput, "PUncooperativeBeforeLock");
  pUncoopBefore = json_object_get_double(jobj);
  jobj = json_object_object_get(jpreinput, "PUncooperativeAfterLock");
  pUncoopAfter = json_object_get_double(jobj);
  jobj = json_object_object_get(jpreinput, "PercentageRWithholding");
  RWithholding = json_object_get_double(jobj);
  jobj = json_object_object_get(jpreinput, "Gini");
  gini = json_object_get_int(jobj);
  jobj = json_object_object_get(jpreinput, "SigmaTopology");
  sigmaTopology = json_object_get_int(jobj);
  jobj = json_object_object_get(jpreinput, "PercentageSameDest");
  sameDest = json_object_get_double(jobj);
  jobj = json_object_object_get(jpreinput, "SigmaAmount");
  sigmaAmount = json_object_get_double(jobj);
  jobj = json_object_object_get(jpreinput, "Capacity");
  capacityPerChannel = json_object_get_int64(jobj);


  endTime = nPayments*paymentMean*1000;

  //printf("%ld\n", endTime);

  /* printf("Create random topology from preinput.json? [y/n]\n"); */
  /* scanf("%c", &answer); */
  /* if(answer == 'y') */
  /*   isPreproc=1; */
  /* else */
  /*   isPreproc=0; */

  begin = clock();

  initializeProtocolData(nPeers, nChannels, pUncoopBefore, pUncoopAfter, RWithholding, gini, sigmaTopology, capacityPerChannel, isPreprocTopology);
  initializeSimulatorData(nPayments, paymentMean, sameDest, sigmaAmount, isPreprocPayments);


  statsInitialize();

  initializeDijkstra();

  end = clock();
  timeSpent = (double) (end - begin)/CLOCKS_PER_SEC;
  printf("Time consumed by initialization: %lf\n", timeSpent);


  begin = clock();
  clock_gettime(CLOCK_MONOTONIC, &start);
  initializeThreads();
  end = clock();
  clock_gettime(CLOCK_MONOTONIC, &finish);
  timeSpent = (double) (end - begin)/CLOCKS_PER_SEC;
  printf("Time consumed by dijkstra executions (normal): %lf\n", timeSpent);
  timeSpent = finish.tv_sec - start.tv_sec;
  printf("Time consumed by dijkstra executions (threads): %lf\n", timeSpent);

  //  floydWarshall();

  return endTime;

}


int main(int argc, char* argv[]) {
  Event* event;
  //  Peer* peer;
  //ChannelInfo* channelInfo;
  //Channel* channel;
  //Payment* payment;
  clock_t  begin, end;
  double timeSpent=0.0;
  uint64_t endTime;
  int preproc;

  if(argc!=2) {
    printf("Command line error\n");
    return -1;
  }

  preproc = atoi(argv[1]);

  endTime = readPreInputAndInitialize(preproc);


  /*
  payment = arrayGet(payments, 7);
  if(payment==NULL) printf("NULL\n");
  printf("Payment %ld,%ld,%ld,%ld\n", payment->ID, payment->sender, payment->receiver, payment->amount);

  peer = arrayGet(peers, 0);
  printf("Peer %ld %d\n", peer->ID, peer->withholdsR);

  channelInfo = arrayGet(channelInfos, 10);
  printf("ChannelInfo %ld %ld %ld %ld %ld %ld %d\n", channelInfo->ID, channelInfo->channelDirection1, channelInfo->channelDirection2, channelInfo->peer1, channelInfo->peer2, channelInfo->capacity, channelInfo->latency);


  channel = arrayGet(channels, channelIndex-1);
  printf("Channel %ld %ld %ld %ld %ld %d %d %d\n", channel->ID, channel->channelInfoID, channel->otherChannelDirectionID, channel->counterparty, channel->balance, channel->policy.feeBase, channel->policy.feeProportional, channel->policy.timelock);
  */

  //  csvWriteInput();
  //jsonWriteInput();

  begin = clock();
  while(heapLen(events) != 0) {
    event = heapPop(events, compareEvent);
    simulatorTime = event->time;

    if(simulatorTime >= endTime) {
      break;
    }

    switch(event->type){
    case FINDROUTE:
      findRoute(event);
      break;
    case SENDPAYMENT:
      sendPayment(event);
      break;
    case FORWARDPAYMENT:
      forwardPayment(event);
      break;
    case RECEIVEPAYMENT:
      receivePayment(event);
      break;
    case FORWARDSUCCESS:
      forwardSuccess(event);
      break;
    case RECEIVESUCCESS:
      receiveSuccess(event);
      break;
    case FORWARDFAIL:
      forwardFail(event);
      break;
    case RECEIVEFAIL:
      receiveFail(event);
      break;
    default:
      printf("ERROR wrong event type\n");
    }
  }

  end = clock();
  timeSpent = (double) (end - begin)/CLOCKS_PER_SEC;
  printf("Time consumed by simulator events: %lf\n", timeSpent);

  //  statsComputeBatchMeans(endTime);

  //printPayments();
  //jsonWriteOutput();

  csvWriteOutput();

  printf("DIJKSTRA CALLS: %ld\n", nDijkstra);

  return 0;
}


/*

// test json writer
int main() {
  long i, nP, nC;
  Peer* peer;
  long sender, receiver;
  Payment *payment;
  Event *event;
  double amount;
  Channel* channel;


  nP = 5;
  nC = 2;

  initializeSimulatorData();
  initializeProtocolData(nP, nC);
  statsInitialize();

  for(i=0; i<nPeers; i++) {
    peer = createPeer(peerIndex,5);
    hashTablePut(peers, peer->ID, peer);
  }


  for(i=1; i<4; i++) {
    connectPeers(i-1, i);
  }
  connectPeers(1, 4);

  jsonWriteInput();

  sender = 0;
  receiver = 3;
  amount = 0.1;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, 0.0, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);

  sender = 0;
  receiver = 3;
  amount = 0.1;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, 0.0, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);

  //this payment fails for peer 2 non cooperative
  sender = 0;
  receiver = 3;
  amount = 0.1;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, 0.0, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);

  //this payment fails for peer 1 non cooperative
  sender = 0;
  receiver = 4;
  amount = 0.1;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, 0.0, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);


 while(heapLen(events) != 0 ) {
    event = heapPop(events, compareEvent);
    simulatorTime = event->time;

    switch(event->type){
    case FINDROUTE:
      findRoute(event);
      break;
    case SENDPAYMENT:
      sendPayment(event);
      break;
    case FORWARDPAYMENT:
      forwardPayment(event);
      break;
    case RECEIVEPAYMENT:
      receivePayment(event);
      break;
    case FORWARDSUCCESS:
      forwardSuccess(event);
      break;
    case RECEIVESUCCESS:
      receiveSuccess(event);
      break;
    case FORWARDFAIL:
      forwardFail(event);
      break;
    case RECEIVEFAIL:
      receiveFail(event);
      break;
    default:
      printf("Error wrong event type\n");
    }
  }


  printPayments();

  jsonWriteOutput();

  return 0;

}

*/


/*
//test statsUpdateLockedFundCost 
int main() {
  long i, nP, nC;
  Peer* peer;
  long sender, receiver;
  Payment *payment;
  Event *event;
  double amount;
  Channel* channel;


  nP = 4;
  nC = 2;

  initializeSimulatorData();
  initializeProtocolData(nP, nC);
  statsInitialize();

  for(i=0; i<nPeers; i++) {
    peer = createPeer(peerIndex,5);
    hashTablePut(peers, peer->ID, peer);
  }


  for(i=1; i<4; i++) {
    connectPeers(i-1, i);
  }


  // failed payment for peer 2 non cooperative in forwardPayment
  sender = 0;
  receiver = 3;
  amount = 0.1;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, 0.0, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);


 while(heapLen(events) != 0 ) {
    event = heapPop(events, compareEvent);
    simulatorTime = event->time;

    switch(event->type){
    case FINDROUTE:
      findRoute(event);
      break;
    case SENDPAYMENT:
      sendPayment(event);
      break;
    case FORWARDPAYMENT:
      forwardPayment(event);
      break;
    case RECEIVEPAYMENT:
      receivePayment(event);
      break;
    case FORWARDSUCCESS:
      forwardSuccess(event);
      break;
    case RECEIVESUCCESS:
      receiveSuccess(event);
      break;
    case FORWARDFAIL:
      forwardFail(event);
      break;
    case RECEIVEFAIL:
      receiveFail(event);
      break;
    default:
      printf("Error wrong event type\n");
    }
  }


  printPayments();
  printStats();


  return 0;

}
*/

/*
//test statsComputePaymentTime 
int main() {
  long i, nP, nC;
  Peer* peer;
  long sender, receiver;
  Payment *payment;
  Event *event;
  double amount;
  Channel* channel;

  
  nP = 4;
  nC = 2;

  initializeSimulatorData();
  initializeProtocolData(nP, nC);
  statsInitialize();

  for(i=0; i<nPeers; i++) {
    peer = createPeer(peerIndex,5);
    hashTablePut(peers, peer->ID, peer);
  }


  for(i=1; i<4; i++) {
    connectPeers(i-1, i);
  }


  //succeed payment
  sender = 0;
  receiver = 3;
  amount = 0.1;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, 0.0, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);

  // failed payment for peer 2 non cooperative in forwardPayment
  sender = 0;
  receiver = 3;
  amount = 0.1;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, 0.0, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);


 while(heapLen(events) != 0 ) {
    event = heapPop(events, compareEvent);
    simulatorTime = event->time;

    switch(event->type){
    case FINDROUTE:
      findRoute(event);
      break;
    case SENDPAYMENT:
      sendPayment(event);
      break;
    case FORWARDPAYMENT:
      forwardPayment(event);
      break;
    case RECEIVEPAYMENT:
      receivePayment(event);
      break;
    case FORWARDSUCCESS:
      forwardSuccess(event);
      break;
    case RECEIVESUCCESS:
      receiveSuccess(event);
      break;
    case FORWARDFAIL:
      forwardFail(event);
      break;
    case RECEIVEFAIL:
      receiveFail(event);
      break;
    default:
      printf("Error wrong event type\n");
    }
  }


  printPayments();
  printStats();


  return 0;

}
*/
/*

//test statsUpdatePayments 
int main() {
  long i, nP, nC;
  Peer* peer;
  long sender, receiver;
  Payment *payment;
  Event *event;
  double amount;
  Channel* channel;

  
  nP = 7;
  nC = 2;

  initializeSimulatorData();
  initializeProtocolData(nP, nC);
  statsInitialize();

  for(i=0; i<nPeers; i++) {
    peer = createPeer(peerIndex,5);
    hashTablePut(peers, peer->ID, peer);
  }


  for(i=1; i<4; i++) {
    connectPeers(i-1, i);
  }

  connectPeers(0, 4);
  connectPeers(4, 5);

  //succeed payment
  sender = 0;
  receiver = 5;
  amount = 0.1;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, 0.0, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);

  // failed payment for no path
  sender = 0;
  receiver = 6;
  amount = 0.1;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, 0.0, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);

  // succeeded payment but uncoop node in forwardsuccess (uncoop if paymentID==2 && peerID==1) 
  sender = 0;
  receiver = 3;
  amount = 0.1;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, 0.0, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);

  //failed payment due to uncoop node in forwardpayment (uncoop if paymentID==3 and peerID==2)
  sender = 0;
  receiver = 3;
  amount = 0.1;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, 0.0, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);


 while(heapLen(events) != 0 ) {
    event = heapPop(events, compareEvent);
    simulatorTime = event->time;

    switch(event->type){
    case FINDROUTE:
      findRoute(event);
      break;
    case SENDPAYMENT:
      sendPayment(event);
      break;
    case FORWARDPAYMENT:
      forwardPayment(event);
      break;
    case RECEIVEPAYMENT:
      receivePayment(event);
      break;
    case FORWARDSUCCESS:
      forwardSuccess(event);
      break;
    case RECEIVESUCCESS:
      receiveSuccess(event);
      break;
    case FORWARDFAIL:
      forwardFail(event);
      break;
    case RECEIVEFAIL:
      receiveFail(event);
      break;
    default:
      printf("Error wrong event type\n");
    }
  }


  printPayments();
  printStats();


  return 0;
}
*/

/*
//test channels not present 
int main() {
  long i, nP, nC;
  Peer* peer;
  long sender, receiver;
  Payment *payment;
  Event *event;
  double amount;
  Channel* channel;

  
  nP = 5;
  nC = 2;

  initializeSimulatorData();
  initializeProtocolData(nP, nC);

  for(i=0; i<nPeers; i++) {
    peer = createPeer(peerIndex,5);
    hashTablePut(peers, peer->ID, peer);
  }


  for(i=1; i<4; i++) {
    connectPeers(i-1, i);
  }

  connectPeers(1, 4);


  //test is!Present in forwardSuccess
  connectPeers(1, 4);

  sender = 0;
  receiver = 4;
  amount = 0.1;
  simulatorTime = 0.0;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, simulatorTime, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);

  //for this payment only peer 1 must be not cooperative
  sender = 0;
  receiver = 3;
  amount = 0.1;
  simulatorTime = 0.05;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, simulatorTime, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);
  // end test is!Present in forwardSuccess



  //test is!Present in forwardFail
  connectPeers(1, 4);

  channel = arrayGet(channels, 6);
  channel->balance = 0.0;

  sender = 0;
  receiver = 4;
  amount = 0.1;
  simulatorTime = 0.0;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, simulatorTime, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);

  //for this payment only peer 1 must be not cooperative
  sender = 0;
  receiver = 3;
  amount = 0.1;
  simulatorTime = 0.05;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, simulatorTime, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);
  // end test is!Present in forwardFail




  //test is!Present in forwardPayment

  //for this payment peer 2 must be non-cooperative
  sender = 0;
  receiver = 3;
  amount = 0.1;
  simulatorTime = 0.0;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, 0.0, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);

  
  sender = 0;
  receiver = 3;
  amount = 0.1;
  //  simulatorTime = 0.2;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, 0.1, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);
  // end test is!Present in forwardPayment


  
  //test is!Present in receiveFail

  //for this payment peer 3 must be non-cooperative
  sender = 0;
  receiver = 3;
  amount = 0.1;
  simulatorTime = 0.0;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, 0.0, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);

  
  sender = 0;
  receiver = 3;
  amount = 0.1;
  //  simulatorTime = 0.2;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, 0.2, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);
  // end test is!Present in receiveFail
  


 while(heapLen(events) != 0 ) {
    event = heapPop(events, compareEvent);

    switch(event->type){
    case FINDROUTE:
      findRoute(event);
      break;
    case SENDPAYMENT:
      sendPayment(event);
      break;
    case FORWARDPAYMENT:
      forwardPayment(event);
      break;
    case RECEIVEPAYMENT:
      receivePayment(event);
      break;
    case FORWARDSUCCESS:
      forwardSuccess(event);
      break;
    case RECEIVESUCCESS:
      receiveSuccess(event);
      break;
    case FORWARDFAIL:
      forwardFail(event);
      break;
    case RECEIVEFAIL:
      receiveFail(event);
      break;
    default:
      printf("Error wrong event type\n");
    }
  }


  printPayments();


  return 0;
}
*/

/*
//test peer not cooperatives
int main() {
  long i, nP, nC;
  Peer* peer;
  long sender, receiver;
  Payment *payment;
  Event *event;
  double amount;
  Channel* channel;

  //test peer 2 not cooperative before/after lock
  nP = 6;
  nC = 2;

  initializeSimulatorData();
  initializeProtocolData(nP, nC);

  for(i=0; i<nPeers; i++) {
    peer = createPeer(peerIndex,5);
    hashTablePut(peers, peer->ID, peer);
  }


  for(i=1; i<5; i++) {
    connectPeers(i-1, i);
  }

 
  connectPeers(1, 5);
  connectPeers(5, 3);
  channel = arrayGet(channels, 8);
  channel->policy.timelock = 5;
  

  sender = 0;
  receiver = 4;
  amount = 1.0;
  simulatorTime = 0.0;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, simulatorTime, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);



 while(heapLen(events) != 0 ) {
    event = heapPop(events, compareEvent);

    switch(event->type){
    case FINDROUTE:
      findRoute(event);
      break;
    case SENDPAYMENT:
      sendPayment(event);
      break;
    case FORWARDPAYMENT:
      forwardPayment(event);
      break;
    case RECEIVEPAYMENT:
      receivePayment(event);
      break;
    case FORWARDSUCCESS:
      forwardSuccess(event);
      break;
    case RECEIVESUCCESS:
      receiveSuccess(event);
      break;
    case FORWARDFAIL:
      forwardFail(event);
      break;
    case RECEIVEFAIL:
      receiveFail(event);
      break;
    default:
      printf("Error wrong event type\n");
    }
  }


  printBalances();


  return 0;
}
*/
/*
//test payments

int main() {
  long i, nP, nC;
  Peer* peer;
  long sender, receiver;
  Payment *payment;
  Event *event;
  double amount;
  Channel* channel;


  nP = 5;
  nC = 2;

  initializeSimulatorData();
  initializeProtocolData(nP, nC);

  for(i=0; i<nPeers; i++) {
    peer = createPeer(peerIndex,5);
    hashTablePut(peers, peer->ID, peer);
  }


  for(i=1; i<5; i++) {
    connectPeers(i-1, i);
  }

  

   
  //test fail
  channel = arrayGet(channels, 6);
  channel->balance = 0.5;
  //end test fail
  

  
  //test ignoredChannels hop
  channel = arrayGet(channels, 6);
  channel->balance = 0.5;
  connectPeers(3,4);
  channel = arrayGet(channels, 8);
  channel->policy.timelock = 5;
  //end test ignoredChannels hop
  

  
  //test ignoredChannels sender
  channel = arrayGet(channels, 0);
  channel->balance = 0.5;
  connectPeers(0,1);
  channel = arrayGet(channels, 8);
  channel->policy.timelock = 5;
  //end test ignoredChannels sender
  

  //test full payment
  sender = 0;
  receiver = 4;
  amount = 1.0;
  simulatorTime = 0.0;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, simulatorTime, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);
  //end test full payment


  
  //test two payments: success and fail
  sender = 0;
  receiver = 4;
  amount = 1.0;
  simulatorTime = 0.0;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, simulatorTime, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);

  sender = 4;
  receiver = 0;
  amount = 4.0;
  simulatorTime = 0.0;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, simulatorTime, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);
  //end test two payments
  

  
  //test payment without hops
  sender = 0;
  receiver = 1;
  amount = 1.0;
  simulatorTime = 0.0;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, simulatorTime, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);
  //end test payment without hops
  

  
  //test more payments from same source
  sender = 0;
  receiver = 4;
  amount = 0.3;
  simulatorTime = 0.0;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, simulatorTime, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);
  
  sender = 0;
  receiver = 4;
  amount = 0.3;
  simulatorTime = 0.0;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, simulatorTime, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);

  sender = 0;
  receiver = 4;
  amount = 0.3;
  simulatorTime = 0.0;
  payment = createPayment(paymentIndex, sender, receiver, amount);
  hashTablePut(payments, payment->ID, payment);
  event = createEvent(eventIndex, simulatorTime, FINDROUTE, sender, payment->ID);
  events = heapInsert(events, event, compareEvent);
   //end test more payments from same source
   

  


 while(heapLen(events) != 0 ) {
    event = heapPop(events, compareEvent);

    switch(event->type){
    case FINDROUTE:
      findRoute(event);
      break;
    case SENDPAYMENT:
      sendPayment(event);
      break;
    case FORWARDPAYMENT:
      forwardPayment(event);
      break;
    case RECEIVEPAYMENT:
      receivePayment(event);
      break;
    case FORWARDSUCCESS:
      forwardSuccess(event);
      break;
    case RECEIVESUCCESS:
      receiveSuccess(event);
      break;
    case FORWARDFAIL:
      forwardFail(event);
      break;
    case RECEIVEFAIL:
      receiveFail(event);
      break;
    default:
      printf("Error wrong event type\n");
    }
  }

  //TODO: stampare ordinatamente le balances per testare correttezza
  printBalances();


  return 0;
}

*/

/*HashTable* peers, *channels, *channelInfos;
long nPeers, nChannels;
 
//test trasformPathIntoRoute
int main() {
  PathHop* pathHop;
  Array *ignored;
  long i, sender, receiver;
  long fakeIgnored = -1;
  Route* route;
  Array* routeHops, *pathHops;
  RouteHop* routeHop;
  Peer* peer;
  Channel* channel;
  ChannelInfo * channelInfo;

  ignored = arrayInitialize(1);
  ignored = arrayInsert(ignored, &fakeIgnored);

  peers = hashTableInitialize(2);
  channels = hashTableInitialize(2);
  channelInfos= hashTableInitialize(2);

  nPeers=5;

  for(i=0; i<nPeers; i++) {
      peer = createPeer(5);
      hashTablePut(peers, peer->ID, peer);
    }

    for(i=1; i<5; i++) {
      connectPeers(i-1, i);
    }

    pathHops=dijkstra(0, 4, 1.0, ignored, ignored );
    route = transformPathIntoRoute(pathHops, 1.0, 5);
    printf("Route/n");
    if(route==NULL) {
      printf("Null route/n");
      return 0;
    }

  for(i=0; i < arrayLen(route->routeHops); i++) {
    routeHop = arrayGet(route->routeHops, i);
    pathHop = routeHop->pathHop;
    printf("HOP %ld\n", i);
    printf("(Sender, Receiver, Channel) = (%ld, %ld, %ld)\n", pathHop->sender, pathHop->receiver, pathHop->channel);
    printf("Amount to forward: %lf\n", routeHop->amountToForward);
    printf("Timelock: %d\n\n", routeHop->timelock);
  }

  printf("Total amount: %lf\n", route->totalAmount);
  printf("Total fee: %lf\n", route->totalFee);
  printf("Total timelock: %d\n", route->totalTimelock);

  return 0;
}
*/
/*
// Test Yen

HashTable* peers, *channels, *channelInfos;
long nPeers, nChannels;

int main() {
  Array *paths;
  Array* path;
  PathHop* hop;
  long i, j;
  Peer* peer;
  Channel* channel;
  ChannelInfo * channelInfo;

  peers = hashTableInitialize(2);
  channels = hashTableInitialize(2);
  channelInfos= hashTableInitialize(2);

  nPeers=8;

  

  for(i=0; i<nPeers; i++) {
    peer = createPeer(5);
    hashTablePut(peers, peer->ID, peer);
  }

  Policy policy;
  policy.fee=0.0;
  policy.timelock=1.0;

  for(i=1; i<5; i++) {
    connectPeers(i-1, i);
  }

  connectPeers(0, 5);
  connectPeers(5,6);
  connectPeers(6,4);

  connectPeers(0,7);
  connectPeers(7,4);




  long *currChannelID;
  for(i=0; i<nPeers; i++) {
    peer = arrayGet(peers, i);
    //    printf("%ld ", arrayLen(peer->channel));
    for(j=0; j<arrayLen(peer->channel); j++) {
      currChannelID=arrayGet(peer->channel, j);
      if(currChannelID==NULL) {
        printf("null\n");
        continue;
      }
      channel = arrayGet(channels, *currChannelID);
      printf("Peer %ld is connected to peer %ld through channel %ld\n", i, channel->counterparty, channel->ID);
      }
  }



  printf("\nYen\n");
  paths=findPaths(0, 4, 0.0);
  printf("%ld\n", arrayLen(paths));
  for(i=0; i<arrayLen(paths); i++) {
    printf("\n");
     path = arrayGet(paths, i);
     for(j=0;j<arrayLen(path); j++) {
       hop = arrayGet(path, j);
       printf("(Sender, Receiver, Channel) = (%ld, %ld, %ld) ", hop->sender, hop->receiver, hop->channel);
     }
  }

  return 0;
}
*/


//test dijkstra
/* int main() { */
/*   Array *hops; */
/*   PathHop* hop; */
/*   Array *ignored; */
/*   long i, sender, receiver; */
/*   long fakeIgnored = -1; */
/*   Peer* peer[3]; */
/*   ChannelInfo* channelInfo; */
/*   Channel* channel; */
/*   Policy policy; */


/*   ignored = arrayInitialize(1); */
/*   ignored = arrayInsert(ignored, &fakeIgnored); */

/*   peers = arrayInitialize(5); */
/*   channels = arrayInitialize(4); */
/*   channelInfos = arrayInitialize(2); */

/*   for(i=0; i<5; i++){ */
/*     peer[i] = createPeerPostProc(i, 0); */
/*     peers = arrayInsert(peers, peer[i]);  */
/*   } */

/*   channelInfo = createChannelInfoPostProc(0, 0, 1, 0, 1, 100, 0); */
/*   channelInfos = arrayInsert(channelInfos, channelInfo); */
/*   channelInfo = createChannelInfoPostProc(1, 2, 3, 1, 2, 100, 0); */
/*   channelInfos = arrayInsert(channelInfos, channelInfo); */


/*   policy.feeBase = 1; */
/*   policy.feeProportional =1; */
/*   policy.minHTLC = 0; */
/*   policy.timelock = 1; */

/*   channel = createChannelPostProc(0, 0, 1, 1, 50, policy); */
/*   channels = arrayInsert(channels, channel); */
/*   peer[0]->channel = arrayInsert(peer[0]->channel, &channel->ID); */

/*   channel = createChannelPostProc(1, 0, 0, 0, 50, policy); */
/*   channels = arrayInsert(channels, channel); */
/*   peer[1]->channel = arrayInsert(peer[1]->channel, &channel->ID); */

/*   channel = createChannelPostProc(2, 1, 3, 2, 50, policy); */
/*   channels = arrayInsert(channels, channel); */
/*   peer[1]->channel = arrayInsert(peer[1]->channel, &channel->ID); */

/*   channel = createChannelPostProc(3, 1, 2, 1, 50, policy); */
/*   channels = arrayInsert(channels, channel); */
/*   peer[2]->channel = arrayInsert(peer[2]->channel, &channel->ID); */


/*   initializeDijkstra(); */
/*   printf("\nDijkstra\n"); */

/*   sender = 2; */
/*   receiver = 0; */
/*   hops=dijkstra(sender, receiver, 10, ignored, ignored ); */
/*   printf("From node %ld to node %ld\n", sender, receiver); */
/*   if(hops!=NULL){ */
/*     for(i=0; i<arrayLen(hops); i++) { */
/*       hop = arrayGet(hops, i); */
/*       printf("(Sender, Receiver, Channel) = (%ld, %ld, %ld) ", hop->sender, hop->receiver, hop->channel); */
/*     } */
/*   } */
/*   else  */
/*     printf("nix\n"); */

/*   /\* sender = 0; *\/ */
/*   /\* receiver = 1; *\/ */
/*   /\* hops=dijkstra(sender, receiver, 0.0, ignored, ignored ); *\/ */
/*   /\* printf("From node %ld to node %ld\n", sender, receiver); *\/ */
/*   /\* for(i=0; i<arrayLen(hops); i++) { *\/ */
/*   /\*   hop = arrayGet(hops, i); *\/ */
/*   /\*   printf("(Sender, Receiver, Channel) = (%ld, %ld, %ld) ", hop->sender, hop->receiver, hop->channel); *\/ */
/*   /\* } *\/ */
/*   /\* printf("\n"); *\/ */

/*   /\* sender = 0; *\/ */
/*   /\* receiver = 0; *\/ */
/*   /\* printf("From node %ld to node %ld\n", sender, receiver); *\/ */
/*   /\* hops=dijkstra(sender,receiver, 0.0, ignored, ignored ); *\/ */
/*   /\* if(hops != NULL) { *\/ */
/*   /\*   for(i=0; i<arrayLen(hops); i++) { *\/ */
/*   /\*     hop = arrayGet(hops, i); *\/ */
/*   /\*     printf("(Sender, Receiver, Channel) = (%ld, %ld, %ld) ", hop->sender, hop->receiver, hop->channel); *\/ */
/*   /\*   } *\/ */
/*   /\* } *\/ */
/*   /\* printf("\n"); *\/ */


/*   return 0; */
/* } */

/*
int main() {
  Array *a;
  long i, *data;

  a=arrayInitialize(10);

  for(i=0; i< 21; i++){
    data = malloc(sizeof(long));
    *data = i;
   a = arrayInsert(a,data);
  }

  for(i=0; i<21; i++) {
    data = arrayGet(a,i);
    if(data!=NULL) printf("%ld ", *data);
    else printf("null ");
  }

  printf("\n%ld ", a->size);

  return 0;
}*/

/*
int main() {
  HashTable *ht;
  long i;
  Event *e;
  long N=100000;

  ht = initializeHashTable(10);

  for(i=0; i<N; i++) {
    e = malloc(sizeof(Event));
    e->time=0.0;
    e->ID=i;
    strcpy(e->type, "Send");

    put(ht, e->ID, e);

}

  long listDim[10]={0};
  Element* el;
  for(i=0; i<10;i++) {
    el =ht->table[i];
    while(el!=NULL){
      listDim[i]++;
      el = el->next;
    }
  }

  for(i=0; i<10; i++)
   printf("%ld ", listDim[i]);

  printf("\n");

  return 0;
}


/*
int main() {

  const gsl_rng_type * T;
  gsl_rng * r;

  int i, n = 10;
  double mu = 0.1;


  gsl_rng_env_setup();

  T = gsl_rng_default;
  r = gsl_rng_alloc (T);


  for (i = 0; i < n; i++)
    {
      double  k = gsl_ran_exponential (r, mu);
      printf (" %lf", k);
    }

  printf ("\n");
  gsl_rng_free (r);

  return 0;
   }
*/
