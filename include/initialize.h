#ifndef INITIALIZE_H
#define INITIALIZE_H

#include <stdint.h>
#include "gsl_rng.h"
#include "array.h"
#include "heap.h"
#include "cloth.h"

extern uint64_t simulator_time; //milliseconds
extern struct array* payments;
FILE* csv_payment;

void initialize_payments(struct payments_params pay_params, unsigned int is_preproc);

void initialize();

void create_payments_from_csv(unsigned int is_preproc);

#endif
