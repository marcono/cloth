#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include "gsl_rng.h"
#include "gsl_randist.h"
#include "gsl_math.h"
#include "../include/array.h"
#include "../include/htlc.h"
#include "../include/initialize.h"
#include "../include/event.h"
#include "../include/network.h"

#define MAXMICRO 1E3
#define MINMICRO 1
#define MAXSMALL 1E8
#define MINSMALL 1E3
#define MAXMEDIUM 1E11
#define MINMEDIUM 1E8

long event_index;
struct heap* events;
uint64_t simulator_time;
struct array *payments;

void initialize_events(long n_payments, double payment_mean) {
  long i, sender_id, receiver_id;
  uint64_t  payment_amount=0, event_time=0 ;
  uint32_t next_event_interval;
  unsigned int payment_class;
  double payment_class_p[]= {0.65, 0.2, 0.1, 0.05}, random_double;
  gsl_ran_discrete_t* discrete;
  struct payment *payment;
  struct event* event;

  discrete = gsl_ran_discrete_preproc(4, payment_class_p);


  for(i=0;i<n_payments;i++) {


    do{
      sender_id = gsl_rng_uniform_int(random_generator,node_index);
      receiver_id = gsl_rng_uniform_int(random_generator, node_index);
    } while(sender_id==receiver_id);


    payment_class = gsl_ran_discrete(random_generator, discrete);
    random_double = gsl_rng_uniform(random_generator);
    switch(payment_class) {
    case 0:
      payment_amount = random_double*gsl_pow_uint(10, gsl_rng_uniform_int(random_generator, 3) + 1);
      break;
    case 1:
      payment_amount = random_double*gsl_pow_uint(10, gsl_rng_uniform_int(random_generator, 3) + 3);
       break;
    case 2:
      payment_amount = random_double*gsl_pow_uint(10, gsl_rng_uniform_int(random_generator, 3) + 6);
      break;
    case 3:
      payment_amount = random_double*gsl_pow_uint(10, gsl_rng_uniform_int(random_generator, 3) + 9);
      break;
    }
    payment = create_payment(payment_index, sender_id, receiver_id, payment_amount);
    array_insert(payments, payment);

    next_event_interval = 1000*gsl_ran_exponential(random_generator, payment_mean);
    event_time += next_event_interval;
    event = create_event(event_index, event_time, FINDROUTE, sender_id, payment->id);
    events = heap_insert(events, event, compare_event);
  }

}

void initialize_payments_preproc(struct payments_params pay_params) {
  long i, sender_id, receiver_id;
  uint64_t  payment_amount=0, event_time=0 ;
  uint32_t next_event_interval;
  double payment_class_p[]= {0.7, 0.2, 0.1, 0.0}, same_dest_p[] = {1-pay_params.same_destination, pay_params.same_destination};
  gsl_ran_discrete_t* discrete_amount, *discrete_dest;
  long payment_idIndex=0;
  int base, exp;
  int npay[8]={0};

  csv_payment = fopen("payments.csv", "w");
  if(csv_payment==NULL) {
    printf("ERROR cannot open file payments.csv\n");
    return;
  }
  fprintf(csv_payment, "id,sender,receiver,amount,time\n");


  discrete_amount = gsl_ran_discrete_preproc(4, payment_class_p);
  discrete_dest = gsl_ran_discrete_preproc(2, same_dest_p);

  for(i=0;i<pay_params.n_payments;i++) {


    do{
      sender_id = gsl_rng_uniform_int(random_generator,node_index);
      if(gsl_ran_discrete(random_generator, discrete_dest))
        receiver_id = 500;
      else
        receiver_id = gsl_rng_uniform_int(random_generator, node_index);
    } while(sender_id==receiver_id);



    do{
      exp = gsl_ran_gaussian_tail(random_generator, 3, pay_params.sigma_amount);
    } while(exp>8);

    ++npay[exp-3];

    base = gsl_rng_uniform_int(random_generator, 8)+1;

    payment_amount = base*gsl_pow_int(10,exp);

    next_event_interval = 1000*gsl_ran_exponential(random_generator, pay_params.payment_mean);
    event_time += next_event_interval;

    fprintf(csv_payment, "%ld,%ld,%ld,%ld,%ld\n", payment_idIndex++, sender_id, receiver_id, payment_amount, event_time );

  }

  /* for(i=0; i<8; i++) */
  /*   printf("%d, %d\n", i+3, npay[i]); */

  //  exit(-1);

  fclose(csv_payment);


  //printf("change payments and press enter\n");
  //scanf("%*c");

}

void create_payments_from_csv(unsigned int is_preproc) {
  struct payment* payment;
  struct event* event;
  char row[256];
  long id, sender, receiver;
  uint64_t amount, time;

  csv_payment = fopen("payments.csv", "r");
  if(csv_payment==NULL) {
    printf("ERROR cannot open file payments.csv\n");
    exit(-1);
  }

  fgets(row, 256, csv_payment);
  while(fgets(row, 256, csv_payment) != NULL) {
    sscanf(row, "%ld,%ld,%ld,%ld,%ld", &id, &sender, &receiver, &amount, &time);
    payment = create_payment(id, sender, receiver, amount);
    array_insert(payments, payment);
    event = create_event(event_index, time, FINDROUTE, sender, payment->id);
    events = heap_insert(events, event, compare_event);
  }

  fclose(csv_payment);
}

void initialize_payments(struct payments_params pay_params, unsigned int is_preproc ) {
  event_index = 0;
  simulator_time = 1;

  payments = array_initialize(pay_params.n_payments);
  events = heap_initialize(pay_params.n_payments*10);

  if(is_preproc)
    initialize_payments_preproc(pay_params);


  create_payments_from_csv(is_preproc);
}



