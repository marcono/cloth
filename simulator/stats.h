#ifndef STATS_H
#define STATS_H
#include "../protocol/protocol.h"
#include "../utils/array.h"
#include <stdint.h>

long* totalPayments;
long* succeededPayments;
long* failedPaymentsUncoop;
long* failedPaymentsNoPath;
long* failedPaymentsNoBalance;
uint64_t* lockedFundCost;
double* avgTimeCoop;
double*avgTimeUncoop;
double* avgRouteLen;
long currentBatch;
Array* batchPayments;

typedef struct statsMean{
  double totalPayments;
  double succeededPayments;
  double failedPaymentsUncoop;
  double failedPaymentsNoPath;
  double failedPaymentsNoBalance;
  double lockedFundCost;
  double avgTimeCoop;
  double avgTimeUncoop;
  double avgRouteLen;
} StatsMean;

void statsInitialize();

void printStats();

void statsUpdatePayments(Payment* payment);

void statsUpdateLockedFundCost(Array* routeHops, long channelID);

void jsonWriteOutput();

#endif
