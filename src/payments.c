#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include "gsl_rng.h"
#include "gsl_randist.h"
#include "gsl_math.h"
#include "../include/array.h"
#include "../include/htlc.h"
#include "../include/payments.h"
#include "../include/network.h"

#define MAXMICRO 1E3
#define MINMICRO 1
#define MAXSMALL 1E8
#define MINSMALL 1E3
#define MAXMEDIUM 1E11
#define MINMEDIUM 1E8


struct payment* new_payment(long id, long sender, long receiver, uint64_t amount, uint64_t start_time) {
  struct payment * p;

  p = malloc(sizeof(struct payment));
  p->id=id;
  p->sender= sender;
  p->receiver = receiver;
  p->amount = amount;
  p->start_time = start_time;
  p->route = NULL;
  p->is_success = 0;
  p->uncoop_after = 0;
  p->uncoop_before=0;
  p->is_timeout = 0;
  p->end_time = 0;
  p->attempts = 0;

  return p;
}



void initialize_payments_preproc(struct payments_params pay_params, long n_nodes, gsl_rng * random_generator) {
  long i, sender_id, receiver_id;
  uint64_t  payment_amount=0, payment_time=0, next_payment_interval ;
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
      sender_id = gsl_rng_uniform_int(random_generator,n_nodes);
      if(gsl_ran_discrete(random_generator, discrete_dest))
        receiver_id = 500;
      else
        receiver_id = gsl_rng_uniform_int(random_generator, n_nodes);
    } while(sender_id==receiver_id);



    do{
      exp = gsl_ran_gaussian_tail(random_generator, 3, pay_params.sigma_amount);
    } while(exp>8);

    ++npay[exp-3];

    base = gsl_rng_uniform_int(random_generator, 8)+1;

    payment_amount = base*gsl_pow_int(10,exp);

    next_payment_interval = 1000*gsl_ran_exponential(random_generator, pay_params.payment_mean);
    payment_time += next_payment_interval;

    fprintf(csv_payment, "%ld,%ld,%ld,%ld,%ld\n", payment_idIndex++, sender_id, receiver_id, payment_amount, payment_time );

  }

  /* for(i=0; i<8; i++) */
  /*   printf("%d, %d\n", i+3, npay[i]); */

  //  exit(-1);

  fclose(csv_payment);


  //printf("change payments and press enter\n");
  //scanf("%*c");

}

struct array* generate_payments_from_file(struct payments_params pay_params, unsigned int is_preproc) {
  struct payment* payment;
  char row[256];
  long id, sender, receiver;
  uint64_t amount, time;
  struct array* payments;

  csv_payment = fopen("payments.csv", "r");
  if(csv_payment==NULL) {
    printf("ERROR cannot open file payments.csv\n");
    exit(-1);
  }

  payments = array_initialize(pay_params.n_payments);

  fgets(row, 256, csv_payment);
  while(fgets(row, 256, csv_payment) != NULL) {
    sscanf(row, "%ld,%ld,%ld,%"SCNu64",%"SCNu64"", &id, &sender, &receiver, &amount, &time);
    payment = new_payment(id, sender, receiver, amount, time);
    array_insert(payments, payment);
  }
  fclose(csv_payment);

  return payments;
}

struct array* initialize_payments(struct payments_params pay_params, unsigned int is_preproc, long n_nodes, gsl_rng* random_generator) {
  if(is_preproc)
    initialize_payments_preproc(pay_params, n_nodes, random_generator);

  return generate_payments_from_file(pay_params, is_preproc);
}



