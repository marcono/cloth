#include <stdio.h>
#include <json-c/json.h>
#include <gsl/gsl_cdf.h>
#include <math.h>
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
  maxTimeCoop = GC_MALLOC(sizeof(uint64_t)*NBATCH);
  minTimeCoop = GC_MALLOC(sizeof(uint64_t)*NBATCH);
  maxTimeUncoop = GC_MALLOC(sizeof(uint64_t)*NBATCH);
  minTimeUncoop = GC_MALLOC(sizeof(uint64_t)*NBATCH);
  maxRouteLen = GC_MALLOC(sizeof(int)*NBATCH);
  minRouteLen = GC_MALLOC(sizeof(int)*NBATCH);

  batchPayments = arrayInitialize(paymentIndex/NBATCH);

  for(i=0;i<NBATCH;i++) {
    totalPayments[i] = succeededPayments[i] = failedPaymentsUncoop[i] = failedPaymentsNoPath[i] = failedPaymentsNoBalance[i] = lockedFundCost[i] = maxTimeCoop[i] = minTimeCoop[i] = maxTimeUncoop[i] = minTimeUncoop[i] = maxRouteLen[i] = minRouteLen[i] = 0;
    avgTimeCoop[i] = avgTimeUncoop[i] = avgRouteLen[i] = 0.0;
  }

  currentBatch = 0;

}

double statsComputePaymentTime(int cooperative, uint64_t* min, uint64_t* max) {
  long i;
  Payment * payment;
  uint64_t currPaymentTime, totalPaymentsTime;
  long nPayments, *paymentID;
  FILE* fpaymentTime;

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
  //  for(i = batchSize*currentBatch; i < currentBatch*batchSize + batchSize; i++)
  for(i=0; i<arrayLen(batchPayments); i++) {
    paymentID = arrayGet(batchPayments, i);

    payment = hashTableGet(payments, *paymentID);
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
  long  nPayments, *paymentID;
  float routeLen;
  int currRouteLen;

  routeLen = nPayments = 0;
  *min=100;
  *max=-1;
  for(i=0; i<arrayLen(batchPayments); i++) {
    paymentID = arrayGet(batchPayments, i);

    payment = hashTableGet(payments, *paymentID);
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
  avgTimeCoop[currentBatch] = statsComputePaymentTime(1, &minTimeCoop[currentBatch], &maxTimeCoop[currentBatch]);
  avgTimeUncoop[currentBatch] = statsComputePaymentTime(0, &minTimeUncoop[currentBatch], &maxTimeUncoop[currentBatch]);
  avgRouteLen[currentBatch] = statsComputeRouteLen(&minRouteLen[currentBatch], &maxRouteLen[currentBatch]);

  arrayDeleteAll(batchPayments);

  ++currentBatch;

}

void statsUpdatePayments(Payment* payment) {
  long batchSize = paymentIndex/NBATCH;

  batchPayments = arrayInsert(batchPayments, &(payment->ID));

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


Stats statsComputeMeans() {
  Stats means;
  long i;

  means.totalPayments = means.succeededPayments = means.failedPaymentsNoPath = means.failedPaymentsUncoop = means.failedPaymentsNoBalance = means.avgRouteLen = means.avgTimeCoop = means.avgTimeUncoop = means.lockedFundCost = 0.0;
  for(i=0;i<NBATCH;i++) {
    means.totalPayments += totalPayments[i];
    means.succeededPayments += succeededPayments[i];
    means.failedPaymentsNoPath += failedPaymentsNoPath[i];
    means.failedPaymentsUncoop += failedPaymentsUncoop[i];
    means.failedPaymentsNoBalance += failedPaymentsNoBalance[i];
    means.avgRouteLen += avgRouteLen[i];
    means.avgTimeCoop += avgTimeCoop[i];
    means.avgTimeUncoop += avgTimeUncoop[i];
    means.minRouteLen += minRouteLen[i];
    means.maxRouteLen += maxRouteLen[i];
    means.minTimeCoop += minTimeCoop[i];
    means.maxTimeCoop += maxTimeCoop[i];
    means.minTimeUncoop += minTimeUncoop[i];
    means.maxTimeUncoop += maxTimeUncoop[i];
    means.lockedFundCost += lockedFundCost[i];
  }

  means.totalPayments /= NBATCH;
  means.succeededPayments /= NBATCH;
  means.failedPaymentsNoPath /= NBATCH;
  means.failedPaymentsUncoop /= NBATCH;
  means.failedPaymentsNoBalance /= NBATCH;
  means.avgRouteLen /= NBATCH;
  means.avgTimeCoop /= NBATCH;
  means.avgTimeUncoop /= NBATCH;
  means.minTimeCoop /= NBATCH;
  means.maxTimeCoop /= NBATCH;
  means.minTimeUncoop /= NBATCH;
  means.maxTimeUncoop /= NBATCH;
  means.minRouteLen /= NBATCH;
  means.maxRouteLen /= NBATCH;
  means.lockedFundCost /= NBATCH;

  return means;
}

Stats statsComputeVariances(Stats means) {
  Stats variances;
  long i;

  variances.totalPayments = variances.succeededPayments = variances.failedPaymentsNoPath = variances.failedPaymentsUncoop = variances.failedPaymentsNoBalance = variances.avgRouteLen = variances.avgTimeCoop = variances.avgTimeUncoop = variances.lockedFundCost = 0.0;
  for(i=0;i<NBATCH;i++) {
    variances.totalPayments += (totalPayments[i] - means.totalPayments)*(totalPayments[i] - means.totalPayments);
    variances.succeededPayments += (succeededPayments[i] - means.succeededPayments)*(succeededPayments[i] - means.succeededPayments);
    variances.failedPaymentsNoPath += (failedPaymentsNoPath[i] - means.failedPaymentsNoPath)*(failedPaymentsNoPath[i] - means.failedPaymentsNoPath);
    variances.failedPaymentsUncoop += (failedPaymentsUncoop[i] - means.failedPaymentsUncoop)*(failedPaymentsUncoop[i] - means.failedPaymentsUncoop);
    variances.failedPaymentsNoBalance += (failedPaymentsNoBalance[i] - means.failedPaymentsNoBalance)*(failedPaymentsNoBalance[i] - means.failedPaymentsNoBalance);
    variances.avgRouteLen += (avgRouteLen[i] - means.avgRouteLen)*(avgRouteLen[i] - means.avgRouteLen);
    variances.avgTimeCoop += (avgTimeCoop[i] - means.avgTimeCoop)*(avgTimeCoop[i] - means.avgTimeCoop);
    variances.avgTimeUncoop += (avgTimeUncoop[i] - means.avgTimeUncoop)*(avgTimeUncoop[i] - means.avgTimeUncoop);
    variances.minRouteLen += (minRouteLen[i] - means.minRouteLen)*(minRouteLen[i] - means.minRouteLen);
    variances.maxRouteLen += (maxRouteLen[i] - means.maxRouteLen)*(maxRouteLen[i] - means.maxRouteLen);
    variances.minTimeCoop += (minTimeCoop[i] - means.minTimeCoop)*(minTimeCoop[i] - means.minTimeCoop);
    variances.maxTimeCoop += (maxTimeCoop[i] - means.maxTimeCoop)*(maxTimeCoop[i] - means.maxTimeCoop);
    variances.minTimeUncoop += (minTimeUncoop[i] - means.minTimeUncoop)*(minTimeUncoop[i] - means.minTimeUncoop);
    variances.maxTimeUncoop += (maxTimeUncoop[i] - means.maxTimeUncoop)*(maxTimeUncoop[i] - means.maxTimeUncoop);
    variances.lockedFundCost += (lockedFundCost[i] - means.lockedFundCost)*(lockedFundCost[i] - means.lockedFundCost);
  }

  variances.totalPayments /= (NBATCH-1);
  variances.succeededPayments /= (NBATCH-1);
  variances.failedPaymentsNoPath /= (NBATCH-1);
  variances.failedPaymentsUncoop /= (NBATCH-1);
  variances.failedPaymentsNoBalance /= (NBATCH-1);
  variances.avgRouteLen /= (NBATCH-1);
  variances.avgTimeCoop /= (NBATCH-1);
  variances.avgTimeUncoop /= (NBATCH-1);
  variances.minRouteLen /= (NBATCH-1);
  variances.maxRouteLen /= (NBATCH-1);
  variances.minTimeUncoop /= (NBATCH-1);
  variances.maxTimeUncoop /= (NBATCH-1);
  variances.minTimeCoop /= (NBATCH-1);
  variances.maxTimeCoop /= (NBATCH-1);
  variances.lockedFundCost /= (NBATCH-1);

  return variances;
}

Stats statsComputeConfidenceMin(Stats means, Stats variances) {
  Stats confidenceMin;
  double confidence, percentile;

  confidence = 0.95;
  percentile =  gsl_cdf_tdist_Qinv(confidence/2, NBATCH -1);

  confidenceMin.succeededPayments = means.succeededPayments - percentile*sqrt(variances.succeededPayments*variances.succeededPayments/NBATCH);
  confidenceMin.failedPaymentsUncoop = means.failedPaymentsUncoop - percentile*sqrt(variances.failedPaymentsUncoop*variances.failedPaymentsUncoop/NBATCH);
  confidenceMin.failedPaymentsNoBalance = means.failedPaymentsNoBalance - percentile*sqrt(variances.failedPaymentsNoBalance*variances.failedPaymentsNoBalance/NBATCH);
  confidenceMin.failedPaymentsNoPath = means.failedPaymentsNoPath - percentile*sqrt(variances.failedPaymentsNoPath*variances.failedPaymentsNoPath/NBATCH);
  confidenceMin.avgTimeCoop = means.avgTimeCoop - percentile*sqrt(variances.avgTimeCoop*variances.avgTimeCoop/NBATCH);
  confidenceMin.minTimeCoop = means.minTimeCoop - percentile*sqrt(variances.minTimeCoop*variances.minTimeCoop/NBATCH);
  confidenceMin.maxTimeCoop = means.maxTimeCoop - percentile*sqrt(variances.maxTimeCoop*variances.maxTimeCoop/NBATCH);
  confidenceMin.avgTimeUncoop = means.avgTimeUncoop - percentile*sqrt(variances.avgTimeUncoop*variances.avgTimeUncoop/NBATCH);
  confidenceMin.minTimeUncoop = means.minTimeUncoop - percentile*sqrt(variances.minTimeUncoop*variances.minTimeUncoop/NBATCH);
  confidenceMin.maxTimeUncoop = means.maxTimeUncoop - percentile*sqrt(variances.maxTimeUncoop*variances.maxTimeUncoop/NBATCH);
  confidenceMin.avgRouteLen = means.avgRouteLen - percentile*sqrt(variances.avgRouteLen*variances.avgRouteLen/NBATCH);
  confidenceMin.minRouteLen = means.minRouteLen - percentile*sqrt(variances.minRouteLen*variances.minRouteLen/NBATCH);
  confidenceMin.maxRouteLen = means.maxRouteLen - percentile*sqrt(variances.maxRouteLen*variances.maxRouteLen/NBATCH);
  confidenceMin.lockedFundCost = means.lockedFundCost - percentile*sqrt(variances.lockedFundCost*variances.lockedFundCost/NBATCH);

  return confidenceMin;
}

Stats statsComputeConfidenceMax(Stats means, Stats variances) {
  Stats confidenceMax;
  double confidence, percentile;

  confidence = 0.95;
  percentile =  gsl_cdf_tdist_Qinv(confidence/2, NBATCH -1);

  confidenceMax.succeededPayments = means.succeededPayments + percentile*sqrt(variances.succeededPayments*variances.succeededPayments/NBATCH);
  confidenceMax.failedPaymentsUncoop = means.failedPaymentsUncoop + percentile*sqrt(variances.failedPaymentsUncoop*variances.failedPaymentsUncoop/NBATCH);
  confidenceMax.failedPaymentsNoBalance = means.failedPaymentsNoBalance + percentile*sqrt(variances.failedPaymentsNoBalance*variances.failedPaymentsNoBalance/NBATCH);
  confidenceMax.failedPaymentsNoPath = means.failedPaymentsNoPath + percentile*sqrt(variances.failedPaymentsNoPath*variances.failedPaymentsNoPath/NBATCH);
  confidenceMax.avgTimeCoop = means.avgTimeCoop + percentile*sqrt(variances.avgTimeCoop*variances.avgTimeCoop/NBATCH);
  confidenceMax.minTimeCoop = means.minTimeCoop + percentile*sqrt(variances.minTimeCoop*variances.minTimeCoop/NBATCH);
  confidenceMax.maxTimeCoop = means.maxTimeCoop + percentile*sqrt(variances.maxTimeCoop*variances.maxTimeCoop/NBATCH);
  confidenceMax.avgTimeUncoop = means.avgTimeUncoop + percentile*sqrt(variances.avgTimeUncoop*variances.avgTimeUncoop/NBATCH);
  confidenceMax.minTimeUncoop = means.minTimeUncoop + percentile*sqrt(variances.minTimeUncoop*variances.minTimeUncoop/NBATCH);
  confidenceMax.maxTimeUncoop = means.maxTimeUncoop + percentile*sqrt(variances.maxTimeUncoop*variances.maxTimeUncoop/NBATCH);
  confidenceMax.avgRouteLen = means.avgRouteLen + percentile*sqrt(variances.avgRouteLen*variances.avgRouteLen/NBATCH);
  confidenceMax.minRouteLen = means.minRouteLen + percentile*sqrt(variances.minRouteLen*variances.minRouteLen/NBATCH);
  confidenceMax.maxRouteLen = means.maxRouteLen + percentile*sqrt(variances.maxRouteLen*variances.maxRouteLen/NBATCH);
  confidenceMax.lockedFundCost = means.lockedFundCost + percentile*sqrt(variances.lockedFundCost*variances.lockedFundCost/NBATCH);

  return confidenceMax;
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
  struct json_object* joutput, *jmeasure, *jmean, *jvariance, *jconfmin, *jconfmax;
  Stats means, variances, confidenceMin, confidenceMax;

  means = statsComputeMeans();
  variances = statsComputeVariances(means);
  confidenceMin = statsComputeConfidenceMin(means, variances);
  confidenceMax = statsComputeConfidenceMax(means, variances);

  joutput = json_object_new_object();

  jmeasure = json_object_new_int64(means.totalPayments);
  json_object_object_add(joutput, "TotalPayments", jmeasure);

  jmeasure = json_object_new_object();
  jmean = json_object_new_double(means.succeededPayments);
  jvariance = json_object_new_double(variances.succeededPayments);
  jconfmin = json_object_new_double(confidenceMin.succeededPayments);
  jconfmax = json_object_new_double(confidenceMax.succeededPayments);
  json_object_object_add(jmeasure, "Mean", jmean);
  json_object_object_add(jmeasure, "Variance", jvariance);
  json_object_object_add(jmeasure, "ConfidenceIntervalMin", jconfmin);
  json_object_object_add(jmeasure, "ConfidenceIntervalMax", jconfmax);
  json_object_object_add(joutput, "SucceededPayments", jmeasure);

  jmeasure = json_object_new_object();
  jmean = json_object_new_double(means.failedPaymentsUncoop);
  jvariance = json_object_new_double(variances.failedPaymentsUncoop);
  jconfmin = json_object_new_double(confidenceMin.failedPaymentsUncoop);
  jconfmax = json_object_new_double(confidenceMax.failedPaymentsUncoop);
  json_object_object_add(jmeasure, "Mean", jmean);
  json_object_object_add(jmeasure, "Variance", jvariance);
  json_object_object_add(jmeasure, "ConfidenceIntervalMin", jconfmin);
  json_object_object_add(jmeasure, "ConfidenceIntervalMax", jconfmax);
  json_object_object_add(joutput, "FailedPaymentsUncoop", jmeasure);

  jmeasure = json_object_new_object();
  jmean = json_object_new_double(means.failedPaymentsNoBalance);
  jvariance = json_object_new_double(variances.failedPaymentsNoBalance);
  jconfmin = json_object_new_double(confidenceMin.failedPaymentsNoBalance);
  jconfmax = json_object_new_double(confidenceMax.failedPaymentsNoBalance);
  json_object_object_add(jmeasure, "Mean", jmean);
  json_object_object_add(jmeasure, "Variance", jvariance);
  json_object_object_add(jmeasure, "ConfidenceIntervalMin", jconfmin);
  json_object_object_add(jmeasure, "ConfidenceIntervalMax", jconfmax);
  json_object_object_add(joutput, "FailedPaymentsNoBalance", jmeasure);

  jmeasure = json_object_new_object();
  jmean = json_object_new_double(means.failedPaymentsNoPath);
  jvariance = json_object_new_double(variances.failedPaymentsNoPath);
  jconfmin = json_object_new_double(confidenceMin.failedPaymentsNoPath);
  jconfmax = json_object_new_double(confidenceMax.failedPaymentsNoPath);
  json_object_object_add(jmeasure, "Mean", jmean);
  json_object_object_add(jmeasure, "Variance", jvariance);
  json_object_object_add(jmeasure, "ConfidenceIntervalMin", jconfmin);
  json_object_object_add(jmeasure, "ConfidenceIntervalMax", jconfmax);
  json_object_object_add(joutput, "FailedPaymentsNoPath", jmeasure);

  jmeasure = json_object_new_object();
  jmean = json_object_new_double(means.avgTimeCoop);
  jvariance = json_object_new_double(variances.avgTimeCoop);
  jconfmin = json_object_new_double(confidenceMin.avgTimeCoop);
  jconfmax = json_object_new_double(confidenceMax.avgTimeCoop);
  json_object_object_add(jmeasure, "Mean", jmean);
  json_object_object_add(jmeasure, "Variance", jvariance);
  json_object_object_add(jmeasure, "ConfidenceIntervalMin", jconfmin);
  json_object_object_add(jmeasure, "ConfidenceIntervalMax", jconfmax);
  json_object_object_add(joutput, "AvgTimeCoop", jmeasure);

  jmeasure = json_object_new_object();
  jmean = json_object_new_double(means.minTimeCoop);
  jvariance = json_object_new_double(variances.minTimeCoop);
  jconfmin = json_object_new_double(confidenceMin.minTimeCoop);
  jconfmax = json_object_new_double(confidenceMax.minTimeCoop);
  json_object_object_add(jmeasure, "Mean", jmean);
  json_object_object_add(jmeasure, "Variance", jvariance);
  json_object_object_add(jmeasure, "ConfidenceIntervalMin", jconfmin);
  json_object_object_add(jmeasure, "ConfidenceIntervalMax", jconfmax);
  json_object_object_add(joutput, "MinTimeCoop", jmeasure);

  jmeasure = json_object_new_object();
  jmean = json_object_new_double(means.maxTimeCoop);
  jvariance = json_object_new_double(variances.maxTimeCoop);
  jconfmin = json_object_new_double(confidenceMin.maxTimeCoop);
  jconfmax = json_object_new_double(confidenceMax.maxTimeCoop);
  json_object_object_add(jmeasure, "Mean", jmean);
  json_object_object_add(jmeasure, "Variance", jvariance);
  json_object_object_add(jmeasure, "ConfidenceIntervalMin", jconfmin);
  json_object_object_add(jmeasure, "ConfidenceIntervalMax", jconfmax);
  json_object_object_add(joutput, "MaxTimeCoop", jmeasure);

  jmeasure = json_object_new_object();
  jmean = json_object_new_double(means.avgTimeUncoop);
  jvariance = json_object_new_double(variances.avgTimeUncoop);
  jconfmin = json_object_new_double(confidenceMin.avgTimeUncoop);
  jconfmax = json_object_new_double(confidenceMax.avgTimeUncoop);
  json_object_object_add(jmeasure, "Mean", jmean);
  json_object_object_add(jmeasure, "Variance", jvariance);
  json_object_object_add(jmeasure, "ConfidenceIntervalMin", jconfmin);
  json_object_object_add(jmeasure, "ConfidenceIntervalMax", jconfmax);
  json_object_object_add(joutput, "AvgTimeUncoop", jmeasure);

  jmeasure = json_object_new_object();
  jmean = json_object_new_double(means.minTimeUncoop);
  jvariance = json_object_new_double(variances.minTimeUncoop);
  jconfmin = json_object_new_double(confidenceMin.minTimeUncoop);
  jconfmax = json_object_new_double(confidenceMax.minTimeUncoop);
  json_object_object_add(jmeasure, "Mean", jmean);
  json_object_object_add(jmeasure, "Variance", jvariance);
  json_object_object_add(jmeasure, "ConfidenceIntervalMin", jconfmin);
  json_object_object_add(jmeasure, "ConfidenceIntervalMax", jconfmax);
  json_object_object_add(joutput, "MinTimeUncoop", jmeasure);

  jmeasure = json_object_new_object();
  jmean = json_object_new_double(means.maxTimeUncoop);
  jvariance = json_object_new_double(variances.maxTimeUncoop);
  jconfmin = json_object_new_double(confidenceMin.maxTimeUncoop);
  jconfmax = json_object_new_double(confidenceMax.maxTimeUncoop);
  json_object_object_add(jmeasure, "Mean", jmean);
  json_object_object_add(jmeasure, "Variance", jvariance);
  json_object_object_add(jmeasure, "ConfidenceIntervalMin", jconfmin);
  json_object_object_add(jmeasure, "ConfidenceIntervalMax", jconfmax);
  json_object_object_add(joutput, "MaxTimeUncoop", jmeasure);

  jmeasure = json_object_new_object();
  jmean = json_object_new_double(means.avgRouteLen);
  jvariance = json_object_new_double(variances.avgRouteLen);
  jconfmin = json_object_new_double(confidenceMin.avgRouteLen);
  jconfmax = json_object_new_double(confidenceMax.avgRouteLen);
  json_object_object_add(jmeasure, "Mean", jmean);
  json_object_object_add(jmeasure, "Variance", jvariance);
  json_object_object_add(jmeasure, "ConfidenceIntervalMin", jconfmin);
  json_object_object_add(jmeasure, "ConfidenceIntervalMax", jconfmax);
  json_object_object_add(joutput, "AvgRouteLen", jmeasure);

  jmeasure = json_object_new_object();
  jmean = json_object_new_double(means.minRouteLen);
  jvariance = json_object_new_double(variances.minRouteLen);
  jconfmin = json_object_new_double(confidenceMin.minRouteLen);
  jconfmax = json_object_new_double(confidenceMax.minRouteLen);
  json_object_object_add(jmeasure, "Mean", jmean);
  json_object_object_add(jmeasure, "Variance", jvariance);
  json_object_object_add(jmeasure, "ConfidenceIntervalMin", jconfmin);
  json_object_object_add(jmeasure, "ConfidenceIntervalMax", jconfmax);
  json_object_object_add(joutput, "MinRouteLen", jmeasure);

  jmeasure = json_object_new_object();
  jmean = json_object_new_double(means.maxRouteLen);
  jvariance = json_object_new_double(variances.maxRouteLen);
  jconfmin = json_object_new_double(confidenceMin.maxRouteLen);
  jconfmax = json_object_new_double(confidenceMax.maxRouteLen);
  json_object_object_add(jmeasure, "Mean", jmean);
  json_object_object_add(jmeasure, "Variance", jvariance);
  json_object_object_add(jmeasure, "ConfidenceIntervalMin", jconfmin);
  json_object_object_add(jmeasure, "ConfidenceIntervalMax", jconfmax);
  json_object_object_add(joutput, "MaxRouteLen", jmeasure);

  jmeasure = json_object_new_object();
  jmean = json_object_new_double(means.lockedFundCost);
  jvariance = json_object_new_double(variances.lockedFundCost);
  jconfmin = json_object_new_double(confidenceMin.lockedFundCost);
  jconfmax = json_object_new_double(confidenceMax.lockedFundCost);
  json_object_object_add(jmeasure, "Mean", jmean);
  json_object_object_add(jmeasure, "Variance", jvariance);
  json_object_object_add(jmeasure, "ConfidenceIntervalMin", jconfmin);
  json_object_object_add(jmeasure, "ConfidenceIntervalMax", jconfmax);
  json_object_object_add(joutput, "LockedFundCost", jmeasure);

  json_object_to_file_ext("output.json", joutput, JSON_C_TO_STRING_PRETTY);
}
