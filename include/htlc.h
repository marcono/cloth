#ifndef HTLC_H
#define HTLC_H

#include <stdint.h>
#include <pthread.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include "array.h"
#include "routing.h"
#include "cloth.h"
#include "network.h"
#include "payments.h"
#include "event.h"

#define OFFLINELATENCY 3000 //3 seconds waiting for a peer not responding (tcp default retransmission time)


void connect_nodes(long node1, long node2);

int is_present(long element, struct array* long_array);

int is_equal(long *a, long *b);

uint64_t compute_fee(uint64_t amount_to_forward, struct policy policy);

void find_route(struct event* event, struct simulation* simulation, struct network* network);

void send_payment(struct event* event, struct simulation* simulation, struct network* network);

void forward_payment(struct event* event, struct simulation* simulation, struct network* network);

void receive_payment(struct event* event, struct simulation* simulation, struct network* network);

void forward_success(struct event* event, struct simulation* simulation, struct network* network);

void receive_success(struct event* event, struct simulation* simulation, struct network* network);

void forward_fail(struct event* event, struct simulation* simulation, struct network* network);

void receive_fail(struct event* event, struct simulation* simulation, struct network* network);

#endif
