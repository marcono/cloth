#ifndef STATS_H
#define STATS_H
#include "../protocol/protocol.h"
#include <stdint.h>

long totalPayments;
long succeededPayments;
long failedPaymentsUncoop;
long failedPaymentsNoPath;
long failedPaymentsNoBalance;
uint64_t lockedFundCost;

void statsInitialize();

void printStats();

void statsUpdatePayments(Payment* payment);

void statsUpdateLockedFundCost(Array* routeHops, long channelID);

void jsonWriteOutput();

#endif
