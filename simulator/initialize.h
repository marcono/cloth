#ifndef INITIALIZE_H
#define INITIALIZE_H
#include "../utils/array.h"
#include "../utils/heap.h"
#include "gsl_rng.h"
#include <stdint.h>

//TODO: mettere time in un altro file (globalSimulator.h per esempio)
extern uint64_t simulatorTime; //milliseconds
extern Array* payments;
FILE* csvPayment;

void initializeSimulatorData(long nPayments, double paymentMean, double sameDest, double sigmaAmount, unsigned int isPreproc);

void initialize();

void createPaymentsFromCsv(unsigned int isPreproc);

#endif
