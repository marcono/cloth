#ifndef INITIALIZE_H
#define INITIALIZE_H
#include "../utils/hashTable.h"
#include "../utils/heap.h"
#include <gsl/gsl_rng.h>
#include <stdint.h>

//TODO: mettere time in un altro file (globalSimulator.h per esempio)
extern uint64_t simulatorTime; //milliseconds
extern HashTable* payments;
FILE* csvPayment;

void initializeSimulatorData();

void initialize();

void createPaymentsFromCsv();

#endif
