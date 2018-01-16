#ifndef STATS_H
#define STATS_H
#include "../protocol/protocol.h"

long totalPayments;
long succeededPayments;
long failedPaymentsUncoop;
long failedPaymentsNoPath;
double lockedFundCost;

void statsInitialize();

void printStats();

void statsUpdatePayments(Payment* payment);

void statsUpdateLockedFundCost(Array* routeHops, long channelID);

void jsonWriteOutput();

#endif
