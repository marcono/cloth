#include <stdio.h>
#include <json-c/json.h>
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

double statsComputePaymentTime(int cooperative) {
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
    if(cooperative && payment->isAPeerUncoop) continue;
    if(!cooperative && !(payment->isAPeerUncoop)) continue;
    nPayments++;
    currPaymentTime = payment->endTime - payment->startTime;
    if(payment->startTime < 0 || payment->endTime < 0 || currPaymentTime < 0) {
      printf("Error in payment time\n");
      return -1.0;
    }
    printf("%lf\n", currPaymentTime);
    totalPaymentsTime += currPaymentTime;
  }


  if(nPayments==0) return -1.0;

  return totalPaymentsTime / nPayments;
}

float statsComputeRouteLen() {
  long i;
  Payment* payment;
  long  nPayments;
  float routeLen;

  routeLen = nPayments = 0;
  for(i = 0; i < paymentIndex; i++) {
    payment = hashTableGet(payments, i);
    if(payment->route == NULL) continue;
    nPayments++;
    routeLen += arrayLen(payment->route->routeHops);
  }

  printf("%f\n", routeLen);
  printf("%ld\n", nPayments);

  if(nPayments==0) return 0.0;
 
  return (routeLen / (nPayments));

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
  printf("Average payment time cooperative: %lf\n", statsComputePaymentTime(1));
  printf("Average payment time not cooperative: %lf\n", statsComputePaymentTime(0));
  printf("Average route length: %f\n", statsComputeRouteLen());
  printf("Locked fund cost: %lf\n", lockedFundCost);
}

void jsonWriteOutput() {
  double averagePaymentTimeCoop, averagePaymentTimeNotCoop, averageRouteLen;
  struct json_object* joutput;
  struct json_object* jtotpay, *jsuccpay, *jfailpayuncoop, *jfailpaynopath, *jtimecoop, *jtimeuncoop, *jroutelen, *jlockedcost;

  averagePaymentTimeCoop = statsComputePaymentTime(1);
  averagePaymentTimeNotCoop = statsComputePaymentTime(0);
  averageRouteLen = statsComputeRouteLen();

  joutput = json_object_new_object();

  jtotpay = json_object_new_int64(totalPayments);
  jsuccpay = json_object_new_int64(succeededPayments);
  jfailpayuncoop = json_object_new_int64(failedPaymentsUncoop);
  jfailpaynopath = json_object_new_int64(failedPaymentsNoPath);
  jtimecoop = json_object_new_double(averagePaymentTimeCoop);
  jtimeuncoop = json_object_new_double(averagePaymentTimeNotCoop);
  jroutelen = json_object_new_double(averageRouteLen);
  jlockedcost = json_object_new_double(lockedFundCost);

  json_object_object_add(joutput, "TotalPayments", jtotpay);
  json_object_object_add(joutput, "SucceededPayments", jsuccpay);
  json_object_object_add(joutput, "FailedPaymentsUncoop", jfailpayuncoop);
  json_object_object_add(joutput, "FailedPaymentsNoPath", jfailpaynopath);
  json_object_object_add(joutput, "AveragePaymentTimeCoop", jtimecoop);
  json_object_object_add(joutput, "AveragePaymentTimeUncoop", jtimeuncoop);
  json_object_object_add(joutput, "AverageRouteLen", jroutelen);
  json_object_object_add(joutput, "LockedFundCost", jlockedcost);

  json_object_to_file_ext("simulator_output.json", joutput, JSON_C_TO_STRING_PRETTY);

}
