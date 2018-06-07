#ifndef STATS_H
#define STATS_H
#include "../protocol/protocol.h"
#include "../utils/array.h"
#include <stdint.h>
#define TRANSIENT 60000

long* totalPayments;
long* succeededPayments;
long* failedPaymentsUncoop;
long* failedPaymentsNoPath;
long* failedPaymentsNoBalance;
uint64_t* lockedFundCost;
double* avgTimeCoop;
double*avgTimeUncoop;
double* avgRouteLen;
uint64_t* minTimeCoop;
uint64_t* maxTimeCoop;
uint64_t* minTimeUncoop;
uint64_t *maxTimeUncoop;
int* minRouteLen;
int*maxRouteLen;
long currentBatch;
Array* batchPayments;

typedef struct stats{
  double totalPayments;
  double succeededPayments;
  double failedPaymentsUncoop;
  double failedPaymentsNoPath;
  double failedPaymentsNoBalance;
  double lockedFundCost;
  double avgTimeCoop;
  double minTimeCoop;
  double maxTimeCoop;
  double avgTimeUncoop;
  double minTimeUncoop;
  double maxTimeUncoop;
  double avgRouteLen;
  double minRouteLen;
  double maxRouteLen;
} Stats;

void statsInitialize();

void printStats();

void statsUpdatePayments(Payment* payment);

void statsUpdateLockedFundCost(Array* routeHops, long channelID);

void jsonWriteOutput();

void statsComputeBatchMeans(uint64_t endTime);

#endif
