#ifndef HTLC_H
#define HTLC_H

#include <stdint.h>
#include <pthread.h>
#include "gsl_rng.h"
#include "gsl_randist.h"
#include "array.h"
#include "routing.h"
#include "cloth.h"
#include "network.h"
#include "payments.h"

#define FAULTYLATENCY 3000 //3 seconds waiting for a peer not responding (tcp default retransmission time)


void connect_nodes(long node1, long node2);

int is_present(long element, struct array* long_array);

int is_equal(long *a, long *b);

uint64_t compute_fee(uint64_t amount_to_forward, struct policy policy);

void find_route(struct event* event);

void send_payment(struct event* event);

void forward_payment(struct event* event);

void receive_payment(struct event* event);

void forward_success(struct event* event);

void receive_success(struct event* event);

void forward_fail(struct event* event);

void receive_fail(struct event* event);

#endif
