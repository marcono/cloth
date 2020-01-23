#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>

#include "gsl_rng.h"
#include "gsl_randist.h"
#include "gsl/gsl_math.h"

#include "../include/htlc.h"
#include "../include/array.h"
#include "../include/heap.h"
#include "../include/payments.h"
#include "../include/routing.h"
#include "../include/event.h"
#include "../include/thread.h"
#include "../include/network.h"


long payment_index;


int is_present(long element, struct array* long_array) {
  long i, *curr;

  if(long_array==NULL) return 0;

  for(i=0; i<array_len(long_array); i++) {
    curr = array_get(long_array, i);
    if(*curr==element) return 1;
  }

  return 0;
}


int is_equal(long* a, long* b) {
  return *a==*b;
}


void close_channel(long channel_id) {
  long i;
  struct node *node;
  struct channel *channel;
  struct edge* direction1, *direction2;

  channel = array_get(channels, channel_id);
  direction1 = array_get(edges, channel->edge1);
  direction2 = array_get(edges, channel->edge2);

  channel->is_closed = 1;
  direction1->is_closed = 1;
  direction2->is_closed = 1;

  printf("struct channel %ld, struct edge_direction1 %ld, struct edge_direction2 %ld are now closed\n", channel->id, channel->edge1, channel->edge2);

  for(i = 0; i < node_index; i++) {
    node = array_get(nodes, i);
    array_delete(node->open_edges, &(channel->edge1), is_equal);
    array_delete(node->open_edges, &(channel->edge2), is_equal);
  }
}


int is_cooperative_before_lock() {
  return gsl_ran_discrete(random_generator, uncoop_before_discrete);
}

int is_cooperative_after_lock() {
  return gsl_ran_discrete(random_generator, uncoop_after_discrete);
}

uint64_t compute_fee(uint64_t amount_to_forward, struct policy policy) {
  uint64_t fee;
  fee = (policy.fee_proportional*amount_to_forward) / 1000000;
  return policy.fee_base + fee;
}

void* dijkstra_thread(void*arg) {
  struct payment * payment;
  struct array* hops;
  long payment_id;
  int *index;
  struct node* node;

  index = (int*) arg;


  while(1) {
    pthread_mutex_lock(&jobs_mutex);
    jobs = pop(jobs, &payment_id);
    pthread_mutex_unlock(&jobs_mutex);

    if(payment_id==-1) return NULL;

    pthread_mutex_lock(&nodes_mutex);
    payment = array_get(payments, payment_id);
    node = array_get(nodes, payment->sender);
    pthread_mutex_unlock(&nodes_mutex);

    printf("DIJKSTRA %ld\n", payment->id);

    hops = dijkstra_p(payment->sender, payment->receiver, payment->amount, node->ignored_nodes,
                     node->ignored_edges, *index);


    paths[payment->id] = hops;
  }

  return NULL;

}

unsigned int is_any_channel_closed(struct array* hops) {
  int i;
  struct edge* edge;
  struct path_hop* hop;

  for(i=0;i<array_len(hops);i++) {
    hop = array_get(hops, i);
    edge = array_get(edges, hop->edge);
    if(edge->is_closed)
      return 1;
  }

  return 0;
}

int is_equal_ignored(struct ignored* a, struct ignored* b){
  return a->id == b->id;
}

void check_ignored(long sender_id){
  struct node* sender;
  struct array* ignored_edges;
  struct ignored* ignored;
  int i;

  sender = array_get(nodes, sender_id);
  ignored_edges = sender->ignored_edges;

  for(i=0; i<array_len(ignored_edges); i++){
    ignored = array_get(ignored_edges, i);

    //register time of newly added ignored edges
    if(ignored->time==0)
      ignored->time = simulator_time;

    //remove decayed ignored edges
    if(simulator_time > 5000 + ignored->time){
      array_delete(ignored_edges, ignored, is_equal_ignored);
    }
  }
}


void find_route(struct event *event) {
  struct payment *payment;
  struct node* node;
  struct array *path_hops;
  struct route* route;
  int final_timelock=9;
  struct event* send_event;
  uint64_t next_event_time;

  printf("FINDROUTE %ld\n", event->payment_id);

  payment = array_get(payments, event->payment_id);
  node = array_get(nodes, payment->sender);

  ++(payment->attempts);

  if(payment->start_time < 1)
    payment->start_time = simulator_time;

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

  //dijkstra parallel OLD version
  /* if(payment->attempts > 0) */
  /*   pthread_create(&tid, NULL, &dijkstra_thread, payment); */

  /* pthread_mutex_lock(&(cond_mutex[payment->id])); */
  /* while(!cond_paths[payment->id]) */
  /*   pthread_cond_wait(&(cond_var[payment->id]), &(cond_mutex[payment->id])); */
  /* cond_paths[payment->id] = 0; */
  /* pthread_mutex_unlock(&(cond_mutex[payment->id])); */

  if(simulator_time > payment->start_time + 60000) {
    payment->end_time = simulator_time;
    payment->is_timeout = 1;
    return;
  }

  check_ignored(payment->sender);

  //dijkstra parallel NEW version
  if(payment->attempts==1) {
    path_hops = paths[payment->id];
    if(path_hops!=NULL)
      if(is_any_channel_closed(path_hops)) {
        path_hops = dijkstra(payment->sender, payment->receiver, payment->amount, node->ignored_nodes,
                            node->ignored_edges);
      }
  }
  else {
    path_hops = dijkstra(payment->sender, payment->receiver, payment->amount, node->ignored_nodes,
                        node->ignored_edges);
  }

  if(path_hops!=NULL)
    if(is_any_channel_closed(path_hops)) {
    path_hops = dijkstra(payment->sender, payment->receiver, payment->amount, node->ignored_nodes,
                                       node->ignored_edges);
    paths[payment->id] = path_hops;
  }
  


  if (path_hops == NULL) {
    printf("No available path\n");
    payment->end_time = simulator_time;
    return;
  }

  route = transform_path_into_route(path_hops, payment->amount, final_timelock);
  if(route==NULL) {
    printf("No available route\n");
    payment->end_time = simulator_time;
    return;
  }

  payment->route = route;

  next_event_time = simulator_time;
  send_event = new_event(event_index, next_event_time, SENDPAYMENT, payment->sender, event->payment_id );
  events = heap_insert(events, send_event, compare_event);

}

struct route_hop *get_route_hop(long node_id, struct array *route_hops, int is_sender) {
  struct route_hop *route_hop;
  long i, index = -1;

  for (i = 0; i < array_len(route_hops); i++) {
    route_hop = array_get(route_hops, i);

    if (is_sender && route_hop->path_hop->sender == node_id) {
      index = i;
      break;
    }

    if (!is_sender && route_hop->path_hop->receiver == node_id) {
      index = i;
      break;
    }
  }

  if (index == -1)
    return NULL;

  return array_get(route_hops, index);
}

int check_policy_forward( struct route_hop* prev_hop, struct route_hop* curr_hop) {
  struct policy policy;
  struct edge* curr_edge, *prev_edge;
  uint64_t fee;

  curr_edge = array_get(edges, curr_hop->path_hop->edge);
  prev_edge = array_get(edges, prev_hop->path_hop->edge);



  fee = compute_fee(curr_hop->amount_to_forward,curr_edge->policy);
  //the check should be: prev_hop->amount_to_forward - fee != curr_hop->amount_to_forward
  if(prev_hop->amount_to_forward - fee != curr_hop->amount_to_forward) {
    printf("ERROR: Fee not respected\n");
    printf("Prev_hop_amount %ld - fee %ld != Curr_hop_amount %ld\n", prev_hop->amount_to_forward, fee, curr_hop->amount_to_forward);
    print_hop(curr_hop);
    return 0;
  }

  if(prev_hop->timelock - prev_edge->policy.timelock != curr_hop->timelock) {
    printf("ERROR: Timelock not respected\n");
    printf("Prev_hop_timelock %d - policy_timelock %d != curr_hop_timelock %d \n",prev_hop->timelock, policy.timelock, curr_hop->timelock);
    print_hop(curr_hop);
    return 0;
  }

  return 1;
}


void add_ignored(long node_id, long ignored_id){
  struct ignored* ignored;
  struct node* node;

  ignored = (struct ignored*)malloc(sizeof(struct ignored));
  ignored->id = ignored_id;
  ignored->time = 0;

  node = array_get(nodes, node_id);
  node->ignored_edges = array_insert(node->ignored_edges, ignored);
}


void send_payment(struct event* event) {
  struct payment* payment;
  uint64_t next_event_time;
  struct route* route;
  struct route_hop* first_route_hop;
  struct edge* next_edge;
  struct channel* channel;
  struct event* next_event;
  enum event_type event_type;
  struct node* node;


  payment = array_get(payments, event->payment_id);
  node = array_get(nodes, event->node_id);
  route = payment->route;
  first_route_hop = array_get(route->route_hops, 0);
  next_edge = array_get(edges, first_route_hop->path_hop->edge);
  channel = array_get(channels, next_edge->channel_id);

  if(!is_present(next_edge->id, node->open_edges)) {
    printf("struct edge %ld has been closed\n", next_edge->id);
    next_event_time = simulator_time;
    next_event = new_event(event_index, next_event_time, FINDROUTE, event->node_id, event->payment_id );
    events = heap_insert(events, next_event, compare_event);
    return;
  }


  if(first_route_hop->amount_to_forward > next_edge->balance) {
    printf("Not enough balance in edge %ld\n", next_edge->id);
    add_ignored(payment->sender, next_edge->id);
    next_event_time = simulator_time;
    next_event = new_event(event_index, next_event_time, FINDROUTE, event->node_id, event->payment_id );
    events = heap_insert(events, next_event, compare_event);
    return;
  }

  next_edge->balance -= first_route_hop->amount_to_forward;


  event_type = first_route_hop->path_hop->receiver == payment->receiver ? RECEIVEPAYMENT : FORWARDPAYMENT;
  next_event_time = simulator_time + channel->latency;
  next_event = new_event(event_index, next_event_time, event_type, first_route_hop->path_hop->receiver, event->payment_id );
  events = heap_insert(events, next_event, compare_event);
}

void forward_payment(struct event *event) {
  struct payment* payment;
  struct route* route;
  struct route_hop* next_route_hop, *previous_route_hop;
  long  next_node_id, prev_node_id;
  enum event_type event_type;
  struct event* next_event;
  uint64_t next_event_time;
  struct channel *prev_channel, *next_channel;
  struct edge* prev_edge, *next_edge;
  int is_policy_respected;
  struct node* node;

  payment = array_get(payments, event->payment_id);
  node = array_get(nodes, event->node_id);
  route = payment->route;
  next_route_hop=get_route_hop(node->id, route->route_hops, 1);
  previous_route_hop = get_route_hop(node->id, route->route_hops, 0);
  prev_edge = array_get(edges, previous_route_hop->path_hop->edge);
  next_edge = array_get(edges, next_route_hop->path_hop->edge);
  prev_channel = array_get(channels, prev_edge->channel_id);
  next_channel = array_get(channels, next_edge->channel_id);

  if(next_route_hop == NULL || previous_route_hop == NULL) {
    printf("ERROR: no route hop\n");
    return;
  }

  if(!is_present(next_route_hop->path_hop->edge, node->open_edges)) {
    printf("struct edge %ld has been closed\n", next_route_hop->path_hop->edge);
    prev_node_id = previous_route_hop->path_hop->sender;
    event_type = prev_node_id == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    next_event_time = simulator_time + prev_channel->latency;
    next_event = new_event(event_index, next_event_time, event_type, prev_node_id, event->payment_id);
    events = heap_insert(events, next_event, compare_event);
    return;
  }

  
  if(!is_cooperative_before_lock()){
    printf("node %ld is not cooperative before lock\n", event->node_id);
    payment->uncoop_before = 1;
    add_ignored(payment->sender, prev_edge->id);

    prev_node_id = previous_route_hop->path_hop->sender;
    event_type = prev_node_id == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    next_event_time = simulator_time + prev_channel->latency + FAULTYLATENCY;
    next_event = new_event(event_index, next_event_time, event_type, prev_node_id, event->payment_id );
    events = heap_insert(events, next_event, compare_event);
    return;
  }


  /* if(!is_cooperative_after_lock()) { */
  /*   printf("struct peer %ld is not cooperative after lock on channel %ld\n", event->peer_id, prev_channel->id); */
  /*   payment->uncoop_after = 1; */
  /*   close_channel(prev_channel->channel_info_id); */

  /*   payment->is_success = 0; */
  /*   payment->end_time = simulator_time; */
  /*   return; */
  /* } */



  is_policy_respected = check_policy_forward(previous_route_hop, next_route_hop);
  if(!is_policy_respected) return;

  if(next_route_hop->amount_to_forward > next_edge->balance ) {
    printf("Not enough balance in edge %ld\n", next_edge->id);
    add_ignored(payment->sender, next_edge->id);

    prev_node_id = previous_route_hop->path_hop->sender;
    event_type = prev_node_id == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    next_event_time = simulator_time + prev_channel->latency;
    next_event = new_event(event_index, next_event_time, event_type, prev_node_id, event->payment_id );
    events = heap_insert(events, next_event, compare_event);
    return;
  }
  next_edge->balance -= next_route_hop->amount_to_forward;


  next_node_id = next_route_hop->path_hop->receiver;
  event_type = next_node_id == payment->receiver ? RECEIVEPAYMENT : FORWARDPAYMENT;
  next_event_time = simulator_time + next_channel->latency;
  next_event = new_event(event_index, next_event_time, event_type, next_node_id, event->payment_id );
  events = heap_insert(events, next_event, compare_event);

}

void receive_payment(struct event* event ) {
  long node_id, prev_node_id;
  struct route* route;
  struct payment* payment;
  struct route_hop* last_route_hop;
  struct edge* forward_edge,*backward_edge;
  struct channel* channel;
  struct event* next_event;
  enum event_type event_type;
  uint64_t next_event_time;
  struct node* node;

  node_id = event->node_id;
  node = array_get(nodes, node_id);
  payment = array_get(payments, event->payment_id);
  route = payment->route;

  last_route_hop = array_get(route->route_hops, array_len(route->route_hops) - 1);
  forward_edge = array_get(edges, last_route_hop->path_hop->edge);
  backward_edge = array_get(edges, forward_edge->other_edge_id);
  channel = array_get(channels, forward_edge->channel_id);

  backward_edge->balance += last_route_hop->amount_to_forward;

  if(!is_cooperative_before_lock()){
    printf("struct node %ld is not cooperative before lock\n", event->node_id);
    payment->uncoop_before = 1;
    add_ignored(payment->sender, forward_edge->id);

    prev_node_id = last_route_hop->path_hop->sender;
    event_type = prev_node_id == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    next_event_time = simulator_time + channel->latency + FAULTYLATENCY;
    next_event = new_event(event_index, next_event_time, event_type, prev_node_id, event->payment_id );
    events = heap_insert(events, next_event, compare_event);
    return;
  }

  payment->is_success = 1;

  prev_node_id = last_route_hop->path_hop->sender;
  event_type = prev_node_id == payment->sender ? RECEIVESUCCESS : FORWARDSUCCESS;
  next_event_time = simulator_time + channel->latency;
  next_event = new_event(event_index, next_event_time, event_type, prev_node_id, event->payment_id);
  events = heap_insert(events, next_event, compare_event);
}


void forward_success(struct event* event) {
  struct route_hop* prev_hop, *next_hop;
  struct payment* payment;
  struct edge* forward_edge, * backward_edge, *next_edge;
  struct channel *prev_channel;
  long prev_node_id;
  struct event* next_event;
  enum event_type event_type;
  struct node* node;
  uint64_t next_event_time;


  payment = array_get(payments, event->payment_id);
  prev_hop = get_route_hop(event->node_id, payment->route->route_hops, 0);
  next_hop = get_route_hop(event->node_id, payment->route->route_hops, 1);
  next_edge = array_get(edges, next_hop->path_hop->edge);
  forward_edge = array_get(edges, prev_hop->path_hop->edge);
  backward_edge = array_get(edges, forward_edge->other_edge_id);
  prev_channel = array_get(channels, forward_edge->channel_id);
  node = array_get(nodes, event->node_id);
 

  if(!is_present(backward_edge->id, node->open_edges)) {
    printf("struct edge %ld is not present\n", prev_hop->path_hop->edge);
    prev_node_id = prev_hop->path_hop->sender;
    event_type = prev_node_id == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    next_event_time = simulator_time + prev_channel->latency;
    next_event = new_event(event_index, next_event_time, event_type, prev_node_id, event->payment_id);
    events = heap_insert(events, next_event, compare_event);
    return;
  }



  /* if(!is_cooperative_after_lock()) { */
  /*   printf("struct node %ld is not cooperative after lock on edge %ld\n", event->node_id, next_edge->id); */
  /*   payment->uncoop_after = 1; */
  /*   close_channel(next_edge->channel_id); */

  /*   payment->is_success = 0; */
  /*   payment->end_time = simulator_time; */

  /*   return; */
  /* } */



  backward_edge->balance += prev_hop->amount_to_forward;


  prev_node_id = prev_hop->path_hop->sender;
  event_type = prev_node_id == payment->sender ? RECEIVESUCCESS : FORWARDSUCCESS;
  next_event_time = simulator_time + prev_channel->latency;
  next_event = new_event(event_index, next_event_time, event_type, prev_node_id, event->payment_id);
  events = heap_insert(events, next_event, compare_event);
}

void receive_success(struct event* event){
  struct payment *payment;
  printf("RECEIVE SUCCESS %ld\n", event->payment_id);
  payment = array_get(payments, event->payment_id);
  payment->end_time = simulator_time;
}


void forward_fail(struct event* event) {
  struct payment* payment;
  struct route_hop* next_hop, *prev_hop;
  struct edge* next_edge, *prev_edge;
  struct channel *prev_channel;
  long prev_node_id;
  struct event* next_event;
  enum event_type event_type;
  struct node* node;
  uint64_t next_event_time;

 
  node = array_get(nodes, event->node_id); 
  payment = array_get(payments, event->payment_id);
  next_hop = get_route_hop(event->node_id, payment->route->route_hops, 1);
  next_edge = array_get(edges, next_hop->path_hop->edge);

  if(is_present(next_edge->id, node->open_edges)) {
    next_edge->balance += next_hop->amount_to_forward;
  }
  else
    printf("struct edge %ld is not present\n", next_hop->path_hop->edge);

  prev_hop = get_route_hop(event->node_id, payment->route->route_hops, 0);
  prev_edge = array_get(edges, prev_hop->path_hop->edge);
  prev_channel = array_get(channels, prev_edge->channel_id);
  prev_node_id = prev_hop->path_hop->sender;
  event_type = prev_node_id == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
  next_event_time = simulator_time + prev_channel->latency;
  next_event = new_event(event_index, next_event_time, event_type, prev_node_id, event->payment_id);
  events = heap_insert(events, next_event, compare_event);
}


void receive_fail(struct event* event) {
  struct payment* payment;
  struct route_hop* first_hop;
  struct edge* next_edge;
  struct event* next_event;
  struct node* node;
  uint64_t next_event_time;

  printf("RECEIVE FAIL %ld\n", event->payment_id);

  payment = array_get(payments, event->payment_id);
  first_hop = array_get(payment->route->route_hops, 0);
  next_edge = array_get(edges, first_hop->path_hop->edge);
  node = array_get(nodes, event->node_id);

  if(is_present(next_edge->id, node->open_edges))
    next_edge->balance += first_hop->amount_to_forward;
  else
    printf("struct edge %ld is not present\n", next_edge->id);

  if(payment->is_success == 1 ) {
    payment->end_time = simulator_time;
    return; //it means that money actually arrived to the destination but a node was not cooperative when forwarding the success
  }

  next_event_time = simulator_time;
  next_event = new_event(event_index, next_event_time, FINDROUTE, payment->sender, payment->id);
  events = heap_insert(events, next_event, compare_event);
}

