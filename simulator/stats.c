#include <stdio.h>
#include "stats.h"
#include "../protocol/protocol.h"

void statsInitialize() {
  totalPayments = succeededPayments = failedPaymentsUncoop = failedPaymentsNoPath = 0;
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

void printStats() {
  printf("\nSTATS\n");
  printf("Total payments: %ld\n", totalPayments);
  printf("Succeed payments: %ld\n", succeededPayments);
  printf("Failed payments for uncooperative peers: %ld\n", failedPaymentsUncoop);
  printf("Failed payments for no path: %ld\n", failedPaymentsNoPath);
}
