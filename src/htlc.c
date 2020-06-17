#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_math.h>

#include "../include/htlc.h"
#include "../include/array.h"
#include "../include/heap.h"
#include "../include/payments.h"
#include "../include/routing.h"
#include "../include/network.h"
#include "../include/event.h"
#include "../include/utils.h"

/* AUXILIARY FUNCTIONS */





uint64_t compute_fee(uint64_t amount_to_forward, struct policy policy) {
  uint64_t fee;
  fee = (policy.fee_proportional*amount_to_forward) / 1000000;
  return policy.fee_base + fee;
}





unsigned int check_balance_and_policy(struct edge* edge, struct route_hop* prev_hop, struct route_hop* next_hop, unsigned int last_hop) {
  uint64_t expected_fee, actual_fee;
  uint32_t timelock_delta;

  if(next_hop->amount_to_forward > edge->balance)
    return 0;

  if(next_hop->amount_to_forward < edge->policy.min_htlc)
    return 0;

  expected_fee = compute_fee(next_hop->amount_to_forward, edge->policy);
  if(prev_hop->amount_to_forward < next_hop->amount_to_forward)
    return 0;
  actual_fee = prev_hop->amount_to_forward - next_hop->amount_to_forward;
  if(actual_fee < expected_fee)
    return 0;

  timelock_delta = last_hop ? FINALTIMELOCK : edge->policy.timelock;
  if(prev_hop->timelock < next_hop->timelock + timelock_delta)
    return 0;

  return 1;
}


struct route_hop *get_route_hop(long node_id, struct array *route_hops, int is_sender) {
  struct route_hop *route_hop;
  long i, index = -1;

  for (i = 0; i < array_len(route_hops); i++) {
    route_hop = array_get(route_hops, i);

    if (is_sender && route_hop->from_node_id == node_id) {
      index = i;
      break;
    }

    if (!is_sender && route_hop->to_node_id == node_id) {
      index = i;
      break;
    }
  }

  if (index == -1)
    return NULL;

  return array_get(route_hops, index);
}

//TODO check correctness
void set_node_pair_result_success(struct element** results, struct route_hop* hop, uint64_t current_time){
  struct node_pair_result* result;

  result = get_by_key(results[hop->from_node_id], hop->to_node_id, is_equal_key_result);

  if(result == NULL){
    result = malloc(sizeof(struct node_pair_result));
    result->to_node_id = hop->to_node_id;
    result->fail_time = 0;
    result->fail_amount = 0;
    result->success_time = current_time;
    result->success_amount = hop->amount_to_forward;
    results[hop->from_node_id] = push(results[hop->from_node_id], result);
    return;
  }

  result->success_time = current_time;
  if(hop->amount_to_forward > result->success_amount)
    result->success_amount = hop->amount_to_forward;
  if(result->fail_time != 0 && result->success_amount > result->fail_amount)
    result->fail_amount = result->success_amount + 1;

}

void process_success_result(struct node* node, struct payment *payment, uint64_t current_time){
  struct route_hop* hop;
  int i;
  struct array* route_hops;

  route_hops = payment->route->route_hops;

  for(i=0; i<array_len(route_hops); i++){
    hop = array_get(route_hops, i);
    set_node_pair_result_success(node->results, hop, current_time);
  }

}

void process_fail_result(struct node* node, struct payment *payment, uint64_t current_time){}

/*HTLC FUNCTIONS*/


void find_route(struct event *event, struct simulation* simulation, struct network* network) {
  struct payment *payment;
  struct node* node;
  struct array *path_hops;
  struct route* route;
  struct event* send_event;
  uint64_t next_event_time;
  enum pathfind_error error;


  payment = event->payment;
  node = array_get(network->nodes, payment->sender);

  //  printf("FINDROUTE %ld\n", payment->id);

  ++(payment->attempts);


  if(simulation->current_time > payment->start_time + 60000) {
    payment->end_time = simulation->current_time;
    payment->is_timeout = 1;
    return;
  }


  if (payment->attempts==1)
    path_hops = paths[payment->id];
  else
    path_hops = dijkstra(payment->sender, payment->receiver, payment->amount, network, 0, &error);


  if (path_hops == NULL) {
    printf("No available path\n");
    payment->end_time = simulation->current_time;
    return;
  }

  route = transform_path_into_route(path_hops, payment->amount, network);
  if(route==NULL) {
    printf("No available route\n");
    payment->end_time = simulation->current_time;
    return;
  }

  payment->route = route;

  next_event_time = simulation->current_time;
  send_event = new_event(next_event_time, SENDPAYMENT, payment->sender, event->payment );
  simulation->events = heap_insert(simulation->events, send_event, compare_event);

}



void send_payment(struct event* event, struct simulation* simulation, struct network *network) {
  struct payment* payment;
  uint64_t next_event_time;
  struct route* route;
  struct route_hop* first_route_hop;
  struct edge* next_edge;
  struct event* next_event;
  enum event_type event_type;
  unsigned int is_next_node_offline;
  struct node* node;

  payment = event->payment;
  route = payment->route;
  node = array_get(network->nodes, event->node_id);
  first_route_hop = array_get(route->route_hops, 0);
  next_edge = array_get(network->edges, first_route_hop->edge_id);

  if(!is_present(next_edge->id, node->open_edges)) {
    printf("ERROR (send_payment): edge %ld is not an edge of node %ld \n", next_edge->id, node->id);
    exit(-1);
  }

  is_next_node_offline = gsl_ran_discrete(simulation->random_generator, network->faulty_node_prob);
  if(is_next_node_offline){
    payment->offline_node_count += 1;
    payment->error.type = OFFLINENODE;
    payment->error.hop = first_route_hop;

    next_event_time = simulation->current_time + OFFLINELATENCY;//prev_channel->latency + FAULTYLATENCY;
    next_event = new_event(next_event_time, RECEIVEFAIL, event->node_id, event->payment);
    simulation->events = heap_insert(simulation->events, next_event, compare_event);
    return;
  }


  if(first_route_hop->amount_to_forward > next_edge->balance) {
    payment->error.type = NOBALANCE;
    payment->error.hop = first_route_hop;
    payment->no_balance_count += 1;
    next_event_time = simulation->current_time;
    next_event = new_event(next_event_time, RECEIVEFAIL, event->node_id, event->payment );
    simulation->events = heap_insert(simulation->events, next_event, compare_event);
    return;
  }

  next_edge->balance -= first_route_hop->amount_to_forward;

  next_edge->tot_flows += 1;

  event_type = first_route_hop->to_node_id == payment->receiver ? RECEIVEPAYMENT : FORWARDPAYMENT;
  next_event_time = simulation->current_time + 100 + gsl_ran_ugaussian(simulation->random_generator);
  next_event = new_event(next_event_time, event_type, first_route_hop->to_node_id, event->payment);
  simulation->events = heap_insert(simulation->events, next_event, compare_event);
}

void forward_payment(struct event *event, struct simulation* simulation, struct network* network) {
  struct payment* payment;
  struct route* route;
  struct route_hop* next_route_hop, *previous_route_hop;
  long  prev_node_id;
  enum event_type event_type;
  struct event* next_event;
  uint64_t next_event_time;
  unsigned int is_next_node_offline;
  struct node* node;
  unsigned int is_last_hop, can_send_htlc;
  int i;
  struct edge *next_edge = NULL;

  payment = event->payment;
  node = array_get(network->nodes, event->node_id);
  route = payment->route;
  next_route_hop=get_route_hop(node->id, route->route_hops, 1);
  previous_route_hop = get_route_hop(node->id, route->route_hops, 0);
  is_last_hop = next_route_hop->to_node_id == payment->receiver;

  if(next_route_hop == NULL || previous_route_hop == NULL) {
    printf("ERROR: no route hop\n");
    exit(-1);
  }

  if(!is_present(next_route_hop->edge_id, node->open_edges)) {
    printf("ERROR (forward_payment): edge %ld is not an edge of node %ld \n", next_edge->id, node->id);
    exit(-1);
  }


  is_next_node_offline = gsl_ran_discrete(simulation->random_generator, network->faulty_node_prob);
  if(!is_last_hop && is_next_node_offline){ //assume that the receiver node is always online
    payment->offline_node_count += 1;
    payment->error.type = OFFLINENODE;
    payment->error.hop = next_route_hop;

    prev_node_id = previous_route_hop->from_node_id;
    event_type = prev_node_id == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    next_event_time = simulation->current_time + 100 + gsl_ran_ugaussian(simulation->random_generator) + OFFLINELATENCY;//prev_channel->latency + FAULTYLATENCY;
    next_event = new_event(next_event_time, event_type, prev_node_id, event->payment);
    simulation->events = heap_insert(simulation->events, next_event, compare_event);
    return;
  }


  /* if(!is_cooperative_after_lock()) { */
  /*   printf("struct peer %ld is not cooperative after lock on channel %ld\n", event->peer_id, prev_channel->id); */
  /*   payment->uncoop_after = 1; */
  /*   close_channel(prev_channel->channel_info_id); */

  /*   payment->is_success = 0; */
  /*   payment->end_time = simulation->current_time; */
  /*   return; */
  /* } */



  /* is_policy_respected = check_policy_forward(previous_route_hop, next_route_hop, network->edges); */
  /* if(!is_policy_respected) return; */

  can_send_htlc = 0;
  for(i=0; i<array_len(node->open_edges); i++) {
    next_edge = array_get(node->open_edges, i);
    if(next_edge->to_node_id != next_route_hop->to_node_id) continue;
    can_send_htlc = check_balance_and_policy(next_edge, previous_route_hop, next_route_hop, is_last_hop);
    if(can_send_htlc) break;
  }

  if(!can_send_htlc){
    payment->error.type = NOBALANCE;
    payment->error.hop = next_route_hop;
    payment->no_balance_count += 1;
    prev_node_id = previous_route_hop->from_node_id;
    event_type = prev_node_id == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    next_event_time = simulation->current_time + 100 + gsl_ran_ugaussian(simulation->random_generator);//prev_channel->latency;
    next_event = new_event(next_event_time, event_type, prev_node_id, event->payment);
    simulation->events = heap_insert(simulation->events, next_event, compare_event);
    return;
  }

  /* if(next_route_hop->amount_to_forward > next_edge->balance ) { */
  /*   printf("Not enough balance in edge %ld\n", next_edge->id); */
  /*   add_ignored_edge(payment->sender, next_edge->id, network->nodes); */

  /*   prev_node_id = previous_route_hop->from_node_id; */
  /*   event_type = prev_node_id == payment->sender ? RECEIVEFAIL : FORWARDFAIL; */
  /*   next_event_time = simulation->current_time + 100 + gsl_ran_ugaussian(simulation->random_generator);//prev_channel->latency; */
  /*   next_event = new_event(next_event_time, event_type, prev_node_id, event->payment); */
  /*   simulation->events = heap_insert(simulation->events, next_event, compare_event); */
  /*   return; */
  /* } */


  next_route_hop->edge_id = next_edge->id;

  next_edge->balance -= next_route_hop->amount_to_forward;

  next_edge->tot_flows += 1;

  event_type = is_last_hop  ? RECEIVEPAYMENT : FORWARDPAYMENT;
  next_event_time = simulation->current_time + 100 + gsl_ran_ugaussian(simulation->random_generator);//next_channel->latency;
  next_event = new_event(next_event_time, event_type, next_route_hop->to_node_id, event->payment);
  simulation->events = heap_insert(simulation->events, next_event, compare_event);
}


void receive_payment(struct event* event, struct simulation* simulation, struct network* network) {
  long  prev_node_id;
  struct route* route;
  struct payment* payment;
  struct route_hop* last_route_hop;
  struct edge* forward_edge,*backward_edge;
  struct event* next_event;
  enum event_type event_type;
  uint64_t next_event_time;
  struct node* node;

  payment = event->payment;
  route = payment->route;
  node = array_get(network->nodes, event->node_id);

  last_route_hop = array_get(route->route_hops, array_len(route->route_hops) - 1);
  forward_edge = array_get(network->edges, last_route_hop->edge_id);
  backward_edge = array_get(network->edges, forward_edge->counter_edge_id);

  if(!is_present(backward_edge->id, node->open_edges)) {
    printf("ERROR (receive_payment): edge %ld is not an edge of node %ld \n", backward_edge->id, node->id);
    exit(-1);
  }


  backward_edge->balance += last_route_hop->amount_to_forward;

  payment->is_success = 1;

  prev_node_id = last_route_hop->from_node_id;
  event_type = prev_node_id == payment->sender ? RECEIVESUCCESS : FORWARDSUCCESS;
  next_event_time = simulation->current_time + 100 + gsl_ran_ugaussian(simulation->random_generator);//channel->latency;
  next_event = new_event(next_event_time, event_type, prev_node_id, event->payment);
  simulation->events = heap_insert(simulation->events, next_event, compare_event);
}


void forward_success(struct event* event, struct simulation* simulation, struct network* network) {
  struct route_hop* prev_hop;
  struct payment* payment;
  struct edge* forward_edge, * backward_edge;
  long prev_node_id;
  struct event* next_event;
  enum event_type event_type;
  struct node* node;
  uint64_t next_event_time;


  payment = event->payment;
  prev_hop = get_route_hop(event->node_id, payment->route->route_hops, 0);
  forward_edge = array_get(network->edges, prev_hop->edge_id);
  backward_edge = array_get(network->edges, forward_edge->counter_edge_id);
  node = array_get(network->nodes, event->node_id);

  if(!is_present(backward_edge->id, node->open_edges)) {
    printf("ERROR (forward_success): edge %ld is not an edge of node %ld \n", backward_edge->id, node->id);
    exit(-1);
  }

  backward_edge->balance += prev_hop->amount_to_forward;

  prev_node_id = prev_hop->from_node_id;
  event_type = prev_node_id == payment->sender ? RECEIVESUCCESS : FORWARDSUCCESS;
  next_event_time = simulation->current_time + 100 + gsl_ran_ugaussian(simulation->random_generator);//prev_channel->latency;
  next_event = new_event(next_event_time, event_type, prev_node_id, event->payment);
  simulation->events = heap_insert(simulation->events, next_event, compare_event);
}


void receive_success(struct event* event, struct simulation *simulation, struct network* network){
  struct node* node;
  struct payment* payment;
  payment = event->payment;
  node = array_get(network->nodes, event->node_id);
  printf("RECEIVE SUCCESS %ld\n", event->payment->id);
  event->payment->end_time = simulation->current_time;
  process_success_result(node, payment, simulation->current_time);
}


void forward_fail(struct event* event, struct simulation* simulation, struct network* network) {
  struct payment* payment;
  struct route_hop* next_hop, *prev_hop;
  struct edge* next_edge;
  long prev_node_id;
  struct event* next_event;
  enum event_type event_type;
  struct node* node;
  uint64_t next_event_time;

  node = array_get(network->nodes, event->node_id);
  payment = event->payment;
  next_hop = get_route_hop(event->node_id, payment->route->route_hops, 1);
  next_edge = array_get(network->edges, next_hop->edge_id);

  if(!is_present(next_edge->id, node->open_edges)) {
    printf("ERROR (forward_fail): edge %ld is not an edge of node %ld \n", next_edge->id, node->id);
    exit(-1);
  }

  next_edge->balance += next_hop->amount_to_forward;
  
  prev_hop = get_route_hop(event->node_id, payment->route->route_hops, 0);
  prev_node_id = prev_hop->from_node_id;
  event_type = prev_node_id == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
  next_event_time = simulation->current_time + 100 + gsl_ran_ugaussian(simulation->random_generator);//prev_channel->latency;
  next_event = new_event(next_event_time, event_type, prev_node_id, event->payment);
  simulation->events = heap_insert(simulation->events, next_event, compare_event);
}


void receive_fail(struct event* event, struct simulation* simulation, struct network* network) {
  struct payment* payment;
  struct route_hop* first_hop, *error_hop;
  struct edge* next_edge;
  struct event* next_event;
  struct node* node;
  uint64_t next_event_time;


  payment = event->payment;
  printf("RECEIVE FAIL %ld\n", payment->id);

  node = array_get(network->nodes, event->node_id);

  error_hop = payment->error.hop;
  if(error_hop->from_node_id != payment->sender){ // if the error occurred in the first hop, the balance hasn't to be updated, since it was not decreased
    first_hop = array_get(payment->route->route_hops, 0);
    next_edge = array_get(network->edges, first_hop->edge_id);
    if(!is_present(next_edge->id, node->open_edges)) {
      printf("ERROR (receive_fail): edge %ld is not an edge of node %ld \n", next_edge->id, node->id);
      exit(-1);
    }
    next_edge->balance += first_hop->amount_to_forward;
  }

  process_fail_result(node, payment, simulation->current_time);

  next_event_time = simulation->current_time;
  next_event = new_event(next_event_time, FINDROUTE, payment->sender, payment);
  simulation->events = heap_insert(simulation->events, next_event, compare_event);
}





//FUNCTIONS FOR UNCOOPERATIVE-AFTER-LOCK BEHAVIOR

/* void close_channel(long channel_id) { */
/*   long i; */
/*   struct node *node; */
/*   struct channel *channel; */
/*   struct edge* direction1, *direction2; */

/*   channel = array_get(channels, channel_id); */
/*   direction1 = array_get(edges, channel->edge1); */
/*   direction2 = array_get(edges, channel->edge2); */

/*   channel->is_closed = 1; */
/*   direction1->is_closed = 1; */
/*   direction2->is_closed = 1; */

/*   printf("struct channel %ld, struct edge_direction1 %ld, struct edge_direction2 %ld are now closed\n", channel->id, channel->edge1, channel->edge2); */

/*   for(i = 0; i < node_index; i++) { */
/*     node = array_get(nodes, i); */
/*     array_delete(node->open_edges, &(channel->edge1), is_equal); */
/*     array_delete(node->open_edges, &(channel->edge2), is_equal); */
/*   } */
/* } */

/* int is_cooperative_after_lock() { */
/*   return gsl_ran_discrete(random_generator, uncoop_after_discrete); */
/* } */

/* unsigned int is_any_channel_closed(struct array* hops) { */
/*   int i; */
/*   struct edge* edge; */
/*   struct path_hop* hop; */

/*   for(i=0;i<array_len(hops);i++) { */
/*     hop = array_get(hops, i); */
/*     edge = array_get(edges, hop->edge); */
/*     if(edge->is_closed) */
/*       return 1; */
/*   } */

/*   return 0; */
/* } */


//old versions for route finding (to be placed in find_route())
  /*
  // dijkstra version
  path_hops = dijkstra(payment->sender, payment->receiver, payment->amount, payment->ignored_nodes,
                      payment->ignored_edges);
*/  

  /* floyd_warshall version
  if(payment->attempts == 0) {
    path_hops = get_path(payment->sender, payment->receiver);
  }
  else {
    path_hops = dijkstra(payment->sender, payment->receiver, payment->amount, payment->ignored_nodes,
                        payment->ignored_edges);
                        }*/

  //dijkstra parallel OLD OLD version
  /* if(payment->attempts > 0) */
  /*   pthread_create(&tid, NULL, &dijkstra_thread, payment); */

  /* pthread_mutex_lock(&(cond_mutex[payment->id])); */
  /* while(!cond_paths[payment->id]) */
  /*   pthread_cond_wait(&(cond_var[payment->id]), &(cond_mutex[payment->id])); */
  /* cond_paths[payment->id] = 0; */
  /* pthread_mutex_unlock(&(cond_mutex[payment->id])); */


  //dijkstra parallel OLD version
  /* if(payment->attempts==1) { */
  /*   path_hops = paths[payment->id]; */
  /*   if(path_hops!=NULL) */
  /*     if(is_any_channel_closed(path_hops)) { */
  /*       path_hops = dijkstra(payment->sender, payment->receiver, payment->amount, node->ignored_nodes, */
  /*                            node->ignored_edges, network, 0); */
  /*     } */
  /* } */
  /* else { */
  /*   path_hops = dijkstra(payment->sender, payment->receiver, payment->amount, node->ignored_nodes, */
  /*                        node->ignored_edges, network, 0); */
  /* } */

  /* if(path_hops!=NULL) */
  /*   if(is_any_channel_closed(path_hops)) { */
  /*     path_hops = dijkstra(payment->sender, payment->receiver, payment->amount, node->ignored_nodes, */
  /*                          node->ignored_edges, network, 0); */
  /*     paths[payment->id] = path_hops; */
  /*   } */

