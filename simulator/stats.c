#include <stdio.h>
#include <json-c/json.h>
#include "../utils/array.h"
#include "../gc-7.2/include/gc.h"
#include "stats.h"
#include "../protocol/protocol.h"
#include "../simulator/initialize.h"
#define NBATCH 30


void statsInitialize() {
  long i;

  totalPayments = GC_MALLOC(sizeof(long)*NBATCH);
  succeededPayments = GC_MALLOC(sizeof(long)*NBATCH);
  failedPaymentsUncoop = GC_MALLOC(sizeof(long)*NBATCH);
  failedPaymentsNoPath = GC_MALLOC(sizeof(long)*NBATCH);
  failedPaymentsNoBalance = GC_MALLOC(sizeof(long)*NBATCH);
  avgTimeCoop = GC_MALLOC(sizeof(double)*NBATCH);
  avgTimeUncoop = GC_MALLOC(sizeof(double)*NBATCH);
  avgRouteLen = GC_MALLOC(sizeof(double)*NBATCH);
  lockedFundCost = GC_MALLOC(sizeof(uint64_t)*NBATCH);

  //batchPayments = arrayInitialize(paymentIndex/NBATCH);

  for(i=0;i<NBATCH;i++) {
    totalPayments[i] = succeededPayments[i] = failedPaymentsUncoop[i] = failedPaymentsNoPath[i] = failedPaymentsNoBalance[i] = lockedFundCost[i] = 0;
    avgTimeCoop[i] = avgTimeUncoop[i] = avgRouteLen[i] = 0.0;
  }

  currentBatch = 0;

}

double statsComputePaymentTime(int cooperative, uint64_t* min, uint64_t* max) {
  long i;
  Payment * payment;
  uint64_t currPaymentTime, totalPaymentsTime;
  long nPayments;
  FILE* fpaymentTime;
  long batchSize  = paymentIndex/NBATCH;

  if(cooperative) {
    fpaymentTime = fopen("payment_time.dat", "w+");
    if(fpaymentTime==NULL) {
      printf("ERROR cannot open payment_time.dat\n");
    }
  }


  nPayments = 0;
  totalPaymentsTime = 0;
  *max = 0;
  *min = UINT64_MAX;
  for(i = batchSize*currentBatch; i < currentBatch*batchSize + batchSize; i++) {
    payment = hashTableGet(payments, i);
    if(payment->route == NULL || !payment->isSuccess) continue;
    if(cooperative && payment->isAPeerUncoop) continue;
    if(!cooperative && !(payment->isAPeerUncoop)) continue;
    nPayments++;
    currPaymentTime = payment->endTime - payment->startTime;
    if(currPaymentTime>*max)
      *max = currPaymentTime;

    if(currPaymentTime<*min)
      *min = currPaymentTime;

    if(payment->startTime < 1 || payment->endTime < 1 || payment->startTime > payment->endTime) {
      printf("ERROR in payment time %"PRIu64" %"PRIu64"\n", payment->startTime, payment->endTime);
      return -1.0;
    }

    if(cooperative)
      fprintf(fpaymentTime, "%ld %"PRIu64"\n", payment->ID, currPaymentTime );
    //    printf("%lf\n", currPaymentTime);
    totalPaymentsTime += currPaymentTime;
  }

  if(nPayments==0){
    *max = -1;
    *min = -1;
    return -1.0;
  }

  if(cooperative)
    fclose(fpaymentTime);

  return totalPaymentsTime / (nPayments*1.0);
}

float statsComputeRouteLen(int* min, int* max) {
  long i;
  Payment* payment;
  long  nPayments;
  float routeLen;
  int currRouteLen;
  long batchSize = paymentIndex/NBATCH;

  routeLen = nPayments = 0;
  *min=100;
  *max=-1;
  for(i = batchSize*currentBatch; i < currentBatch*batchSize + batchSize; i++) {
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

  if(nPayments==0) {
    *max = -1;
    *min = -1;
    return -1.0;
  }

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

  lockedFundCost[currentBatch] += amountLocked*lockTime;
}



void statsComputeBatch() {
  uint64_t minTime, maxTime;
  int minRoute, maxRoute;

  avgTimeCoop[currentBatch] = statsComputePaymentTime(1, &minTime, &maxTime);
  avgTimeUncoop[currentBatch] = statsComputePaymentTime(0, &minTime, &maxTime);
  avgRouteLen[currentBatch] = statsComputeRouteLen(&minRoute, &maxRoute);

  ++currentBatch;

}

void statsUpdatePayments(Payment* payment) {
  long batchSize = paymentIndex/NBATCH;

  //  batchPayments = arrayInsert(batchPayments, &(payment->ID));

  totalPayments[currentBatch]++;
  if(payment->isSuccess) {
    succeededPayments[currentBatch]++;
  }
  else {
    if(payment->route==NULL)
      failedPaymentsNoPath[currentBatch]++;
    else if(payment->isAPeerUncoop)
      failedPaymentsUncoop[currentBatch]++;
    else
      failedPaymentsNoBalance[currentBatch]++;
  }

  if(totalPayments[currentBatch] == batchSize) statsComputeBatch();
}


StatsMean statsComputeMeans() {
  StatsMean statsMean;
  long i;
  
  statsMean.totalPayments = statsMean.succeededPayments = statsMean.failedPaymentsNoPath = statsMean.failedPaymentsUncoop = statsMean.failedPaymentsNoBalance = statsMean.avgRouteLen = statsMean.avgTimeCoop = statsMean.avgTimeUncoop = statsMean.lockedFundCost = 0.0;
  for(i=0;i<NBATCH;i++) {
    statsMean.totalPayments += totalPayments[i];
    statsMean.succeededPayments += succeededPayments[i];
    statsMean.failedPaymentsNoPath += failedPaymentsNoPath[i];
    printf("%ld %ld\n", i, failedPaymentsNoPath[i]);
    statsMean.failedPaymentsUncoop += failedPaymentsUncoop[i];
    statsMean.failedPaymentsNoBalance += failedPaymentsNoBalance[i];
    statsMean.avgRouteLen += avgRouteLen[i];
    statsMean.avgTimeCoop += avgTimeCoop[i];
    statsMean.avgTimeUncoop += avgTimeUncoop[i];
    statsMean.lockedFundCost += lockedFundCost[i];
  }

  statsMean.totalPayments /= NBATCH;
  statsMean.succeededPayments /= NBATCH;
  statsMean.failedPaymentsNoPath /= NBATCH;
  statsMean.failedPaymentsUncoop /= NBATCH;
  statsMean.failedPaymentsNoBalance /= NBATCH;
  statsMean.avgRouteLen /= NBATCH;
  statsMean.avgTimeCoop /= NBATCH;
  statsMean.avgTimeUncoop /= NBATCH;
  statsMean.lockedFundCost /= NBATCH;

  return statsMean;
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
  StatsMean statsMean;

  statsMean = statsComputeMeans();

  //  averagePaymentTimeCoop = statsComputePaymentTime(1, &minPayTimeCoop, &maxPayTimeCoop);
  //averagePaymentTimeNotCoop = statsComputePaymentTime(0, &minPayTimeUncoop, &maxPayTimeUncoop);
  //averageRouteLen = statsComputeRouteLen(&minRouteLen, &maxRouteLen);

  joutput = json_object_new_object();

  jtotpay = json_object_new_int64(statsMean.totalPayments);
  jsuccpay = json_object_new_int64(statsMean.succeededPayments);
  jfailpayuncoop = json_object_new_int64(statsMean.failedPaymentsUncoop);
  jfailpaynobalance = json_object_new_int64(statsMean.failedPaymentsNoBalance);
  jfailpaynopath = json_object_new_int64(statsMean.failedPaymentsNoPath);
  jlockedcost = json_object_new_int64(statsMean.lockedFundCost);

  jtime = json_object_new_object();
  javtimecoop = json_object_new_double(statsMean.avgTimeCoop);
  //jmintimecoop = json_object_new_int64(minPayTimeCoop);
  //jmaxtimecoop = json_object_new_int64(maxPayTimeCoop);
  javtimeuncoop = json_object_new_double(statsMean.avgTimeUncoop);
  //jmintimeuncoop = json_object_new_int64(minPayTimeUncoop);
  //jmaxtimeuncoop = json_object_new_int64(maxPayTimeUncoop);
  json_object_object_add(jtime, "AverageCoop", javtimecoop);
  //json_object_object_add(jtime, "MinCoop", jmintimecoop);
  //json_object_object_add(jtime, "MaxCoop", jmaxtimecoop);
  json_object_object_add(jtime, "AverageUncoop", javtimeuncoop);
  //json_object_object_add(jtime, "MinUncoop", jmintimeuncoop);
  //json_object_object_add(jtime, "MaxUncoop", jmaxtimeuncoop);

  jroutelen = json_object_new_object();
  javroutelen = json_object_new_double(statsMean.avgRouteLen);
  //jminroutelen = json_object_new_int64(minRouteLen);
  //jmaxroutelen = json_object_new_int64(maxRouteLen);
  json_object_object_add(jroutelen, "Average", javroutelen);
  //json_object_object_add(jroutelen, "Min", jminroutelen);
  //json_object_object_add(jroutelen, "Max", jmaxroutelen);


  json_object_object_add(joutput, "TotalPayments", jtotpay);
  json_object_object_add(joutput, "SucceededPayments", jsuccpay);
  json_object_object_add(joutput, "FailedPaymentsUncoop", jfailpayuncoop);
  json_object_object_add(joutput, "FailedPaymentsNoBalance", jfailpaynobalance);
  json_object_object_add(joutput, "FailedPaymentsNoPath", jfailpaynopath);
  json_object_object_add(joutput, "Time", jtime);
  json_object_object_add(joutput, "RouteLength", jroutelen);
  json_object_object_add(joutput, "LockedFundCost", jlockedcost);

  json_object_to_file_ext("output.json", joutput, JSON_C_TO_STRING_PRETTY);

}
