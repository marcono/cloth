#include <stdio.h>
#include <json-c/json.h>
#include "stats.h"
#include "../protocol/protocol.h"

void statsInitialize() {
  totalPayments = succeededPayments = failedPaymentsUncoop = failedPaymentsNoPath = failedPaymentsNoBalance = 0;
  lockedFundCost = 0;
}

void statsUpdatePayments(Payment* payment) {
  totalPayments++;
  if(payment->isSuccess) {
    succeededPayments++;
  }
  else {
    if(payment->route==NULL)
      failedPaymentsNoPath++;
    else if(payment->isAPeerUncoop)
      failedPaymentsUncoop++;
    else
      failedPaymentsNoBalance++;
  }
}

double statsComputePaymentTime(int cooperative, uint64_t* min, uint64_t* max) {
  long i;
  Payment * payment;
  uint64_t currPaymentTime, totalPaymentsTime;
  long nPayments, ID;

  nPayments = 0;
  totalPaymentsTime = 0;
  *max = 0;
  *min = UINT64_MAX;
  for(i = 0; i < paymentIndex; i++) {
    payment = hashTableGet(payments, i);
    if(payment->route == NULL || !payment->isSuccess) continue;
    if(cooperative && payment->isAPeerUncoop) continue;
    if(!cooperative && !(payment->isAPeerUncoop)) continue;
    nPayments++;
    currPaymentTime = payment->endTime - payment->startTime;
    if(currPaymentTime>*max) {
      *max = currPaymentTime;
      ID = payment->ID;
    }
    if(currPaymentTime<*min) 
      *min = currPaymentTime;

    if(payment->startTime < 1 || payment->endTime < 1 || payment->startTime > payment->endTime) {
      printf("ERROR in payment time %"PRIu64" %"PRIu64"\n", payment->startTime, payment->endTime);
      return -1.0;
    }
    //    printf("%lf\n", currPaymentTime);
    totalPaymentsTime += currPaymentTime;
  }

  if(nPayments==0) return 0.0;

  if(cooperative)
    printf("max pay id: %ld\n", ID);

  return totalPaymentsTime / (nPayments*1.0);
}

float statsComputeRouteLen(int* min, int* max) {
  long i;
  Payment* payment;
  long  nPayments;
  float routeLen;
  int currRouteLen;

  routeLen = nPayments = 0;
  *min=100;
  *max=-1;
  for(i = 0; i < paymentIndex; i++) {
    payment = hashTableGet(payments, i);
    if(payment->route == NULL) continue;
    nPayments++;
    currRouteLen = arrayLen(payment->route->routeHops);
    if(currRouteLen < *min)
      *min = currRouteLen;
    if(currRouteLen>*max)
      *max = currRouteLen;
    routeLen += currRouteLen;
  }

  if(nPayments==0) return 0.0;

  return (routeLen / (nPayments));

}

void statsUpdateLockedFundCost(Array* routeHops, long channelID) {
  long i;
  uint64_t amountLocked;
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
/*
void printStats() {
  printf("\nSTATS\n");
  printf("Total payments: %ld\n", totalPayments);
  printf("Succeed payments: %ld\n", succeededPayments);
  printf("Failed payments for uncooperative peers: %ld\n", failedPaymentsUncoop);
  printf("Failed payments for no path: %ld\n", failedPaymentsNoPath);
  printf("Average payment time cooperative: %lf\n", statsComputePaymentTime(1));
  printf("Average payment time not cooperative: %lf\n", statsComputePaymentTime(0));
  printf("Average route length: %f\n", statsComputeRouteLen());
  printf("Locked fund cost: %ld\n", lockedFundCost);
}
*/
void jsonWriteOutput() {
  double averagePaymentTimeCoop, averagePaymentTimeNotCoop, averageRouteLen;
  uint64_t maxPayTimeCoop, minPayTimeCoop, maxPayTimeUncoop, minPayTimeUncoop;
  int minRouteLen, maxRouteLen;
  struct json_object* joutput;
  struct json_object* jtime, *jroutelen;
  struct json_object* jtotpay, *jsuccpay, *jfailpayuncoop, *jfailpaynopath, *jfailpaynobalance, *jlockedcost,
    *javtimecoop, *jmintimecoop, *jmaxtimecoop,  *javtimeuncoop, *jmintimeuncoop, *jmaxtimeuncoop, *javroutelen, *jminroutelen, *jmaxroutelen ;

  averagePaymentTimeCoop = statsComputePaymentTime(1, &minPayTimeCoop, &maxPayTimeCoop);
  averagePaymentTimeNotCoop = statsComputePaymentTime(0, &minPayTimeUncoop, &maxPayTimeUncoop);
  averageRouteLen = statsComputeRouteLen(&minRouteLen, &maxRouteLen);

  joutput = json_object_new_object();

  jtotpay = json_object_new_int64(totalPayments);
  jsuccpay = json_object_new_int64(succeededPayments);
  jfailpayuncoop = json_object_new_int64(failedPaymentsUncoop);
  jfailpaynobalance = json_object_new_int64(failedPaymentsNoBalance);
  jfailpaynopath = json_object_new_int64(failedPaymentsNoPath);
  jlockedcost = json_object_new_int64(lockedFundCost);

  jtime = json_object_new_object();
  javtimecoop = json_object_new_double(averagePaymentTimeCoop);
  jmintimecoop = json_object_new_int64(minPayTimeCoop);
  jmaxtimecoop = json_object_new_int64(maxPayTimeCoop);
  javtimeuncoop = json_object_new_double(averagePaymentTimeNotCoop);
  jmintimeuncoop = json_object_new_int64(minPayTimeUncoop);
  jmaxtimeuncoop = json_object_new_int64(maxPayTimeUncoop);
  json_object_object_add(jtime, "AverageCoop", javtimecoop);
  json_object_object_add(jtime, "MinCoop", jmintimecoop);
  json_object_object_add(jtime, "MaxCoop", jmaxtimecoop);
  json_object_object_add(jtime, "AverageUncoop", javtimeuncoop);
  json_object_object_add(jtime, "MinUncoop", jmintimeuncoop);
  json_object_object_add(jtime, "MaxUncoop", jmaxtimeuncoop);

  jroutelen = json_object_new_object();
  javroutelen = json_object_new_double(averageRouteLen);
  jminroutelen = json_object_new_int64(minRouteLen);
  jmaxroutelen = json_object_new_int64(maxRouteLen);
  json_object_object_add(jroutelen, "Average", javroutelen);
  json_object_object_add(jroutelen, "Min", jminroutelen);
  json_object_object_add(jroutelen, "Max", jmaxroutelen);
  

  json_object_object_add(joutput, "TotalPayments", jtotpay);
  json_object_object_add(joutput, "SucceededPayments", jsuccpay);
  json_object_object_add(joutput, "FailedPaymentsUncoop", jfailpayuncoop);
  json_object_object_add(joutput, "FailedPaymentsNoBalance", jfailpaynobalance);
  json_object_object_add(joutput, "FailedPaymentsNoPath", jfailpaynopath);
  json_object_object_add(joutput, "Time", jtime);
  json_object_object_add(joutput, "RouteLength", jroutelen);
  json_object_object_add(joutput, "LockedFundCost", jlockedcost);

  json_object_to_file_ext("simulator_output.json", joutput, JSON_C_TO_STRING_PRETTY);

}
