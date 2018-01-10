#ifndef STATS_H
#define STATS_H
#include "../protocol/protocol.h"

long totalPayments;
long succeededPayments;
long failedPaymentsUncoop;
long failedPaymentsNoPath;

void statsInitialize();

void printStats();

void statsUpdatePayments(Payment* payment);

#endif
