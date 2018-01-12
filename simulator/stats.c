#include <stdio.h>
#include "stats.h"
#include "../protocol/protocol.h"

void statsInitialize() {
  totalPayments = succeededPayments = failedPaymentsUncoop = failedPaymentsNoPath = 0;
  lockedFundCost = 0.0;
}

void statsUpdatePayments(Payment* payment) {
  totalPayments++;
  if(payment->isSuccess) {
    succeededPayments++;
  }
  else {
    if(payment->isAPeerUncoop)
      failedPaymentsUncoop++;
    else
      failedPaymentsNoPath++;
  }
}

double statsComputePaymentTime() {
  long i;
  Payment * payment;
  double currPaymentTime, totalPaymentsTime;
  double nPayments;

  //TODO: lo si potrebbe fare separato per totalpayments e
  // payments con peer non cooperativi per avere risultati consistenti;
  nPayments = 0;
  totalPaymentsTime = 0.0;
  for(i = 0; i < paymentIndex; i++) {
    payment = hashTableGet(payments, i);
    if(payment->route == NULL) continue;
    nPayments++;
    currPaymentTime = payment->endTime - payment->startTime;
    if(payment->startTime < 0 || payment->endTime < 0 || currPaymentTime < 0) {
      printf("Error in payment time\n");
      return -1.0;
    }
    printf("%lf\n", currPaymentTime);
    totalPaymentsTime += currPaymentTime;
  }

  return totalPaymentsTime / nPayments;
}

float statsComputeRouteLen() {
  long i;
  Payment* payment;
  long routeLen, nPayments;

  routeLen = nPayments = 0;
  for(i = 0; i < paymentIndex; i++) {
    payment = hashTableGet(payments, i);
    if(payment->route == NULL) continue;
    nPayments++;
    routeLen += arrayLen(payment->route->routeHops);
  }

  return (routeLen / (nPayments*1.0) );

}

void statsUpdateLockedFundCost(Array* routeHops, long channelID) {
  long i;
  double amountLocked;
  int lockTime=0;
  RouteHop* hop;

  amountLocked = 0.0;
  for(i = 0 ; i < arrayLen(routeHops); i++) {
    hop = arrayGet(routeHops, i);
    amountLocked += hop->amountToForward;
    if(hop->pathHop->channel == channelID) {
      lockTime = hop->timelock;
      break;
    }
  }

  lockedFundCost += amountLocked*lockTime;
}

void printStats() {
  printf("\nSTATS\n");
  printf("Total payments: %ld\n", totalPayments);
  printf("Succeed payments: %ld\n", succeededPayments);
  printf("Failed payments for uncooperative peers: %ld\n", failedPaymentsUncoop);
  printf("Failed payments for no path: %ld\n", failedPaymentsNoPath);
  printf("Average payment time: %lf\n", statsComputePaymentTime());
  printf("Average route length: %f\n", statsComputeRouteLen());
  printf("Locked fund cost: %lf\n", lockedFundCost);
}
