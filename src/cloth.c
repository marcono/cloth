#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <inttypes.h>

#include <gsl/gsl_rng.h>
#include <gsl/gsl_cdf.h>

#include "../include/payments.h"
#include "../include/heap.h"
#include "../include/array.h"
#include "../include/routing.h"
#include "../include/htlc.h"
#include "../include/list.h"
#include "../include/cloth.h"
#include "../include/network.h"
#include "../include/event.h"

void write_output(struct network* network, struct array* payments) {
  FILE* csv_channel_output, *csv_edge_output, *csv_payment_output, *csv_node_output;
  long i,j, *id;
  struct channel* channel;
  struct edge* edge;
  struct payment* payment;
  struct node* node;
  struct route* route;
  struct array* hops;
  struct route_hop* hop;

  csv_channel_output = fopen("channels_output.csv", "w");
  if(csv_channel_output  == NULL) {
    printf("ERROR cannot open channel_output.csv\n");
    exit(-1);
  }
  fprintf(csv_channel_output, "id,direction1,direction2,node1,node2,capacity,latency,is_closed\n");

  for(i=0; i<array_len(network->channels); i++) {
    channel = array_get(network->channels, i);
    fprintf(csv_channel_output, "%ld,%ld,%ld,%ld,%ld,%ld,%d,%d\n", channel->id, channel->edge1, channel->edge2, channel->node1, channel->node2, channel->capacity, channel->latency, channel->is_closed);
  }

  fclose(csv_channel_output);

  csv_edge_output = fopen("edges_output.csv", "w");
  if(csv_channel_output  == NULL) {
    printf("ERROR cannot open edge_output.csv\n");
    exit(-1);
  }
  fprintf(csv_edge_output, "id,channel,other_direction,counterparty,balance,fee_base,fee_proportional,min_htlc,timelock,is_closed,tot_flows\n");

  for(i=0; i<array_len(network->edges); i++) {
    edge = array_get(network->edges, i);
    fprintf(csv_edge_output, "%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%d,%d,%ld\n", edge->id, edge->channel_id, edge->counter_edge_id, edge->from_node_id, edge->to_node_id, edge->balance, edge->policy.fee_base, edge->policy.fee_proportional, edge->policy.min_htlc, edge->policy.timelock, edge->is_closed, edge->tot_flows);
  }

  fclose(csv_edge_output);

  csv_payment_output = fopen("payments_output.csv", "w");
  if(csv_channel_output  == NULL) {
    printf("ERROR cannot open payment_output.csv\n");
    return;
  }
  fprintf(csv_payment_output, "id,sender,receiver,amount,time,end_time,is_success,uncoop_after,uncoop_before,is_timeout,attempts,route,total_fee\n");

  for(i=0; i<array_len(payments); i++)  {
    payment = array_get(payments, i);
    //TODO: fixme
    fprintf(csv_payment_output, "%ld,%ld,%ld,%"PRIu64",%"PRIu64",%"PRIu64",%u,%u,%u,%u,%d,", payment->id, payment->sender, payment->receiver, payment->amount, payment->start_time, payment->end_time, payment->is_success, 0, payment->offline_node_count, payment->is_timeout, payment->attempts);
    route = payment->route;

    if(route==NULL)
      fprintf(csv_payment_output, "-1");
    else {
      hops = route->route_hops;
      for(j=0; j<array_len(hops); j++) {
        hop = array_get(hops, j);
        if(j==array_len(hops)-1)
          fprintf(csv_payment_output,"%ld,",hop->edge_id);
        else
          fprintf(csv_payment_output,"%ld-",hop->edge_id);
      }
      fprintf(csv_payment_output, "%ld", route->total_fee);
    }
    fprintf(csv_payment_output,"\n");
  }

  fclose(csv_payment_output);


  csv_node_output = fopen("nodes_output.csv", "w");
  if(csv_channel_output  == NULL) {
    printf("ERROR cannot open node_output.csv\n");
    return;
  }
  fprintf(csv_node_output, "id,open_edges,ignored_nodes,ignored_edges\n");

  for(i=0; i<array_len(network->nodes); i++) {
    node = array_get(network->nodes, i);

    fprintf(csv_node_output, "%ld,", node->id);

    if(array_len(node->open_edges)==0)
      fprintf(csv_node_output, "-1");
    else {
      for(j=0; j<array_len(node->open_edges); j++) {
        id = array_get(node->open_edges, j);
        if(j==array_len(node->open_edges)-1)
          fprintf(csv_node_output,"%ld",*id);
        else
          fprintf(csv_node_output,"%ld-",*id);
      }
    }
    fprintf(csv_node_output,"\n");
  }

  fclose(csv_node_output);

}

void read_input(struct network_params* net_params, struct payments_params* pay_params){
  FILE* input_file;
  char *parameter, *value, line[1024];

  input_file = fopen("cloth_input.txt","r");

  if(input_file==NULL){
    fprintf(stderr, "ERROR: cannot open file <cloth_input.txt> in cloth directory.\n");
    exit(-1);
  }

  while(fgets(line, 1024, input_file)){

    parameter = strtok(line, "=");
    value = strtok(NULL, "=");
    if(parameter==NULL || value==NULL){
      fprintf(stderr, "ERROR: wrong format in file <cloth_input.txt>\n");
      exit(-1);
    }

    if(value[0]==' ' || parameter[strlen(parameter)-1]==' '){
      fprintf(stderr, "ERROR: no space allowed after/before <=> character in <cloth_input.txt>. Space detected in parameter <%s>\n", parameter);
      exit(-1);
    }

    value[strlen(value)-1] = '\0';

    if(strcmp(parameter, "generate_network_from_file")==0){
      if(strcmp(value, "true")==0)
        net_params->network_from_file=1;
      else if(strcmp(value, "false")==0)
        net_params->network_from_file=0;
      else{
        fprintf(stderr, "ERROR: wrong value of parameter <%s> in <cloth_input.txt>. Possible values are <true> or <false>\n", parameter);
        exit(-1);
      }
    }
    else if(strcmp(parameter, "nodes_csv")==0){
      strcpy(net_params->nodes_filename, value);
    }
    else if(strcmp(parameter, "channels_csv")==0){
      strcpy(net_params->channels_filename, value);
    }
    else if(strcmp(parameter, "edges_csv")==0){
      strcpy(net_params->edges_filename, value);
    }
    else if(strcmp(parameter, "n_nodes")==0){
      net_params->n_nodes = strtol(value, NULL, 10);
    }
    else if(strcmp(parameter, "n_channels_per_node")==0){
      net_params->n_channels = strtol(value, NULL, 10);
    }
    else if(strcmp(parameter, "sigma_topology")==0){
      net_params->sigma_topology = strtol(value, NULL, 10);
    }
    else if(strcmp(parameter, "capacity_per_channel")==0){
      net_params->capacity_per_channel = strtol(value, NULL, 10);
    }
    else if(strcmp(parameter, "faulty_node_probability")==0){
      net_params->faulty_node_prob = strtod(value, NULL);
    }
    else if(strcmp(parameter, "generate_payments_from_file")==0){
      if(strcmp(value, "true")==0)
        pay_params->payments_from_file=1;
      else if(strcmp(value, "false")==0)
        pay_params->payments_from_file=0;
      else{
        fprintf(stderr, "ERROR: wrong value of parameter <%s> in <cloth_input.txt>. Possible values are <true> or <false>\n", parameter);
        exit(-1);
      }
    }
    else if(strcmp(parameter, "payments_csv")==0){
      strcpy(pay_params->payments_filename, value);
    }
    else if(strcmp(parameter, "payment_mean")==0){
      pay_params->payment_mean = strtod(value, NULL);
    }
    else if(strcmp(parameter, "n_payments")==0){
      pay_params->n_payments = strtol(value, NULL, 10);
    }
    else if(strcmp(parameter, "sigma_amount")==0){
      pay_params->sigma_amount = strtod(value, NULL);
    }
    else{
      fprintf(stderr, "ERROR: unknown parameter <%s>\n", parameter);
      exit(-1);
    }

  }


}


gsl_rng* initialize_random_generator(){
  gsl_rng_env_setup();
  return gsl_rng_alloc (gsl_rng_default);
}


int main() {
  struct event* event;
  clock_t  begin, end;
  double time_spent=0.0;
  struct network_params net_params;
  struct payments_params pay_params;
  struct timespec start, finish;
  struct network *network;
  long n_nodes, n_edges;
  struct array* payments;
  struct simulation* simulation;

  read_input(&net_params, &pay_params);

  simulation = malloc(sizeof(struct simulation));

  simulation->random_generator = initialize_random_generator();
  printf("NETWORK INITIALIZATION\n");
  network = initialize_network(net_params, simulation->random_generator);
  n_nodes = array_len(network->nodes);
  n_edges = array_len(network->edges);
  printf("PAYMENTS INITIALIZATION\n");
  payments = initialize_payments(pay_params,  n_nodes, simulation->random_generator);
  printf("EVENTS INITIALIZATION\n");
  simulation->events = initialize_events(payments);
  initialize_dijkstra(n_nodes, n_edges, payments);

  printf("INITIAL DIJKSTRA THREADS EXECUTION\n");
  clock_gettime(CLOCK_MONOTONIC, &start);
  run_dijkstra_threads(network, payments, 0);
  clock_gettime(CLOCK_MONOTONIC, &finish);
  time_spent = finish.tv_sec - start.tv_sec;
  printf("Time consumed by initial dijkstra executions: %lf\n", time_spent);

  begin = clock();
  simulation->current_time = 1;
  // end_time = 6000;
  while(heap_len(simulation->events) != 0) {
    event = heap_pop(simulation->events, compare_event);
    simulation->current_time = event->time;

    /* if(simulation->current_time >= end_time) { */
    /*   break; */
    /* } */

    switch(event->type){
    case FINDROUTE:
      find_route(event, simulation, network);
      break;
    case SENDPAYMENT:
      send_payment(event, simulation, network);
      break;
    case FORWARDPAYMENT:
      forward_payment(event, simulation, network);
      break;
    case RECEIVEPAYMENT:
      receive_payment(event, simulation, network);
      break;
    case FORWARDSUCCESS:
      forward_success(event, simulation, network);
      break;
    case RECEIVESUCCESS:
      receive_success(event, simulation, network);
      break;
    case FORWARDFAIL:
      forward_fail(event, simulation, network);
      break;
    case RECEIVEFAIL:
      receive_fail(event, simulation, network);
      break;
    case OPENCHANNEL:
      open_channel(network, simulation->random_generator);
      break;
    default:
      printf("ERROR wrong event type\n");
      exit(-1);
    }
  }
  end = clock();

  time_spent = (double) (end - begin)/CLOCKS_PER_SEC;
  printf("Time consumed by simulation events: %lf\n", time_spent);

  write_output(network, payments);

  return 0;
}


/*

// test json writer
int main() {
  long i, n_p, n_c;
  struct peer* peer;
  long sender, receiver;
  struct payment *payment;
  struct event *event;
  double amount;
  struct edge* channel;


  n_p = 5;
  n_c = 2;

  initialize_simulator_data();
  initialize_protocol_data(n_p, n_c);
  stats_initialize();

  for(i=0; i<n_peers; i++) {
    peer = create_peer(peer_index,5);
    hash_table_put(peers, peer->id, peer);
  }


  for(i=1; i<4; i++) {
    connect_peers(i-1, i);
  }
  connect_peers(1, 4);

  json_write_input();

  sender = 0;
  receiver = 3;
  amount = 0.1;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, 0.0, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);

  sender = 0;
  receiver = 3;
  amount = 0.1;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, 0.0, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);

  //this payment fails for peer 2 non cooperative
  sender = 0;
  receiver = 3;
  amount = 0.1;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, 0.0, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);

  //this payment fails for peer 1 non cooperative
  sender = 0;
  receiver = 4;
  amount = 0.1;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, 0.0, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);


 while(heap_len(events) != 0 ) {
    event = heap_pop(events, compare_event);
    simulator_time = event->time;

    switch(event->type){
    case FINDROUTE:
      find_route(event);
      break;
    case SENDPAYMENT:
      send_payment(event);
      break;
    case FORWARDPAYMENT:
      forward_payment(event);
      break;
    case RECEIVEPAYMENT:
      receive_payment(event);
      break;
    case FORWARDSUCCESS:
      forward_success(event);
      break;
    case RECEIVESUCCESS:
      receive_success(event);
      break;
    case FORWARDFAIL:
      forward_fail(event);
      break;
    case RECEIVEFAIL:
      receive_fail(event);
      break;
    default:
      printf("Error wrong event type\n");
    }
  }


  print_payments();

  json_write_output();

  return 0;

}

*/


/*
//test stats_update_locked_fund_cost 
int main() {
  long i, n_p, n_c;
  struct peer* peer;
  long sender, receiver;
  struct payment *payment;
  struct event *event;
  double amount;
  struct edge* channel;


  n_p = 4;
  n_c = 2;

  initialize_simulator_data();
  initialize_protocol_data(n_p, n_c);
  stats_initialize();

  for(i=0; i<n_peers; i++) {
    peer = create_peer(peer_index,5);
    hash_table_put(peers, peer->id, peer);
  }


  for(i=1; i<4; i++) {
    connect_peers(i-1, i);
  }


  // failed payment for peer 2 non cooperative in forward_payment
  sender = 0;
  receiver = 3;
  amount = 0.1;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, 0.0, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);


 while(heap_len(events) != 0 ) {
    event = heap_pop(events, compare_event);
    simulator_time = event->time;

    switch(event->type){
    case FINDROUTE:
      find_route(event);
      break;
    case SENDPAYMENT:
      send_payment(event);
      break;
    case FORWARDPAYMENT:
      forward_payment(event);
      break;
    case RECEIVEPAYMENT:
      receive_payment(event);
      break;
    case FORWARDSUCCESS:
      forward_success(event);
      break;
    case RECEIVESUCCESS:
      receive_success(event);
      break;
    case FORWARDFAIL:
      forward_fail(event);
      break;
    case RECEIVEFAIL:
      receive_fail(event);
      break;
    default:
      printf("Error wrong event type\n");
    }
  }


  print_payments();
  print_stats();


  return 0;

}
*/

/*
//test stats_compute_payment_time 
int main() {
  long i, n_p, n_c;
  struct peer* peer;
  long sender, receiver;
  struct payment *payment;
  struct event *event;
  double amount;
  struct edge* channel;

  
  n_p = 4;
  n_c = 2;

  initialize_simulator_data();
  initialize_protocol_data(n_p, n_c);
  stats_initialize();

  for(i=0; i<n_peers; i++) {
    peer = create_peer(peer_index,5);
    hash_table_put(peers, peer->id, peer);
  }


  for(i=1; i<4; i++) {
    connect_peers(i-1, i);
  }


  //succeed payment
  sender = 0;
  receiver = 3;
  amount = 0.1;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, 0.0, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);

  // failed payment for peer 2 non cooperative in forward_payment
  sender = 0;
  receiver = 3;
  amount = 0.1;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, 0.0, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);


 while(heap_len(events) != 0 ) {
    event = heap_pop(events, compare_event);
    simulator_time = event->time;

    switch(event->type){
    case FINDROUTE:
      find_route(event);
      break;
    case SENDPAYMENT:
      send_payment(event);
      break;
    case FORWARDPAYMENT:
      forward_payment(event);
      break;
    case RECEIVEPAYMENT:
      receive_payment(event);
      break;
    case FORWARDSUCCESS:
      forward_success(event);
      break;
    case RECEIVESUCCESS:
      receive_success(event);
      break;
    case FORWARDFAIL:
      forward_fail(event);
      break;
    case RECEIVEFAIL:
      receive_fail(event);
      break;
    default:
      printf("Error wrong event type\n");
    }
  }


  print_payments();
  print_stats();


  return 0;

}
*/
/*

//test stats_update_payments 
int main() {
  long i, n_p, n_c;
  struct peer* peer;
  long sender, receiver;
  struct payment *payment;
  struct event *event;
  double amount;
  struct edge* channel;

  
  n_p = 7;
  n_c = 2;

  initialize_simulator_data();
  initialize_protocol_data(n_p, n_c);
  stats_initialize();

  for(i=0; i<n_peers; i++) {
    peer = create_peer(peer_index,5);
    hash_table_put(peers, peer->id, peer);
  }


  for(i=1; i<4; i++) {
    connect_peers(i-1, i);
  }

  connect_peers(0, 4);
  connect_peers(4, 5);

  //succeed payment
  sender = 0;
  receiver = 5;
  amount = 0.1;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, 0.0, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);

  // failed payment for no path
  sender = 0;
  receiver = 6;
  amount = 0.1;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, 0.0, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);

  // succeeded payment but uncoop node in forwardsuccess (uncoop if payment_id==2 && peer_id==1) 
  sender = 0;
  receiver = 3;
  amount = 0.1;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, 0.0, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);

  //failed payment due to uncoop node in forwardpayment (uncoop if payment_id==3 and peer_id==2)
  sender = 0;
  receiver = 3;
  amount = 0.1;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, 0.0, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);


 while(heap_len(events) != 0 ) {
    event = heap_pop(events, compare_event);
    simulator_time = event->time;

    switch(event->type){
    case FINDROUTE:
      find_route(event);
      break;
    case SENDPAYMENT:
      send_payment(event);
      break;
    case FORWARDPAYMENT:
      forward_payment(event);
      break;
    case RECEIVEPAYMENT:
      receive_payment(event);
      break;
    case FORWARDSUCCESS:
      forward_success(event);
      break;
    case RECEIVESUCCESS:
      receive_success(event);
      break;
    case FORWARDFAIL:
      forward_fail(event);
      break;
    case RECEIVEFAIL:
      receive_fail(event);
      break;
    default:
      printf("Error wrong event type\n");
    }
  }


  print_payments();
  print_stats();


  return 0;
}
*/

/*
//test channels not present 
int main() {
  long i, n_p, n_c;
  struct peer* peer;
  long sender, receiver;
  struct payment *payment;
  struct event *event;
  double amount;
  struct edge* channel;

  
  n_p = 5;
  n_c = 2;

  initialize_simulator_data();
  initialize_protocol_data(n_p, n_c);

  for(i=0; i<n_peers; i++) {
    peer = create_peer(peer_index,5);
    hash_table_put(peers, peer->id, peer);
  }


  for(i=1; i<4; i++) {
    connect_peers(i-1, i);
  }

  connect_peers(1, 4);


  //test is!Present in forward_success
  connect_peers(1, 4);

  sender = 0;
  receiver = 4;
  amount = 0.1;
  simulator_time = 0.0;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, simulator_time, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);

  //for this payment only peer 1 must be not cooperative
  sender = 0;
  receiver = 3;
  amount = 0.1;
  simulator_time = 0.05;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, simulator_time, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);
  // end test is!Present in forward_success



  //test is!Present in forward_fail
  connect_peers(1, 4);

  channel = array_get(channels, 6);
  channel->balance = 0.0;

  sender = 0;
  receiver = 4;
  amount = 0.1;
  simulator_time = 0.0;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, simulator_time, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);

  //for this payment only peer 1 must be not cooperative
  sender = 0;
  receiver = 3;
  amount = 0.1;
  simulator_time = 0.05;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, simulator_time, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);
  // end test is!Present in forward_fail




  //test is!Present in forward_payment

  //for this payment peer 2 must be non-cooperative
  sender = 0;
  receiver = 3;
  amount = 0.1;
  simulator_time = 0.0;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, 0.0, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);

  
  sender = 0;
  receiver = 3;
  amount = 0.1;
  //  simulator_time = 0.2;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, 0.1, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);
  // end test is!Present in forward_payment


  
  //test is!Present in receive_fail

  //for this payment peer 3 must be non-cooperative
  sender = 0;
  receiver = 3;
  amount = 0.1;
  simulator_time = 0.0;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, 0.0, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);

  
  sender = 0;
  receiver = 3;
  amount = 0.1;
  //  simulator_time = 0.2;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, 0.2, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);
  // end test is!Present in receive_fail
  


 while(heap_len(events) != 0 ) {
    event = heap_pop(events, compare_event);

    switch(event->type){
    case FINDROUTE:
      find_route(event);
      break;
    case SENDPAYMENT:
      send_payment(event);
      break;
    case FORWARDPAYMENT:
      forward_payment(event);
      break;
    case RECEIVEPAYMENT:
      receive_payment(event);
      break;
    case FORWARDSUCCESS:
      forward_success(event);
      break;
    case RECEIVESUCCESS:
      receive_success(event);
      break;
    case FORWARDFAIL:
      forward_fail(event);
      break;
    case RECEIVEFAIL:
      receive_fail(event);
      break;
    default:
      printf("Error wrong event type\n");
    }
  }


  print_payments();


  return 0;
}
*/

/*
//test peer not cooperatives
int main() {
  long i, n_p, n_c;
  struct peer* peer;
  long sender, receiver;
  struct payment *payment;
  struct event *event;
  double amount;
  struct edge* channel;

  //test peer 2 not cooperative before/after lock
  n_p = 6;
  n_c = 2;

  initialize_simulator_data();
  initialize_protocol_data(n_p, n_c);

  for(i=0; i<n_peers; i++) {
    peer = create_peer(peer_index,5);
    hash_table_put(peers, peer->id, peer);
  }


  for(i=1; i<5; i++) {
    connect_peers(i-1, i);
  }

 
  connect_peers(1, 5);
  connect_peers(5, 3);
  channel = array_get(channels, 8);
  channel->policy.timelock = 5;
  

  sender = 0;
  receiver = 4;
  amount = 1.0;
  simulator_time = 0.0;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, simulator_time, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);



 while(heap_len(events) != 0 ) {
    event = heap_pop(events, compare_event);

    switch(event->type){
    case FINDROUTE:
      find_route(event);
      break;
    case SENDPAYMENT:
      send_payment(event);
      break;
    case FORWARDPAYMENT:
      forward_payment(event);
      break;
    case RECEIVEPAYMENT:
      receive_payment(event);
      break;
    case FORWARDSUCCESS:
      forward_success(event);
      break;
    case RECEIVESUCCESS:
      receive_success(event);
      break;
    case FORWARDFAIL:
      forward_fail(event);
      break;
    case RECEIVEFAIL:
      receive_fail(event);
      break;
    default:
      printf("Error wrong event type\n");
    }
  }


  print_balances();


  return 0;
}
*/
/*
//test payments

int main() {
  long i, n_p, n_c;
  struct peer* peer;
  long sender, receiver;
  struct payment *payment;
  struct event *event;
  double amount;
  struct edge* channel;


  n_p = 5;
  n_c = 2;

  initialize_simulator_data();
  initialize_protocol_data(n_p, n_c);

  for(i=0; i<n_peers; i++) {
    peer = create_peer(peer_index,5);
    hash_table_put(peers, peer->id, peer);
  }


  for(i=1; i<5; i++) {
    connect_peers(i-1, i);
  }

  

   
  //test fail
  channel = array_get(channels, 6);
  channel->balance = 0.5;
  //end test fail
  

  
  //test ignored_channels hop
  channel = array_get(channels, 6);
  channel->balance = 0.5;
  connect_peers(3,4);
  channel = array_get(channels, 8);
  channel->policy.timelock = 5;
  //end test ignored_channels hop
  

  
  //test ignored_channels sender
  channel = array_get(channels, 0);
  channel->balance = 0.5;
  connect_peers(0,1);
  channel = array_get(channels, 8);
  channel->policy.timelock = 5;
  //end test ignored_channels sender
  

  //test full payment
  sender = 0;
  receiver = 4;
  amount = 1.0;
  simulator_time = 0.0;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, simulator_time, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);
  //end test full payment


  
  //test two payments: success and fail
  sender = 0;
  receiver = 4;
  amount = 1.0;
  simulator_time = 0.0;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, simulator_time, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);

  sender = 4;
  receiver = 0;
  amount = 4.0;
  simulator_time = 0.0;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, simulator_time, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);
  //end test two payments
  

  
  //test payment without hops
  sender = 0;
  receiver = 1;
  amount = 1.0;
  simulator_time = 0.0;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, simulator_time, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);
  //end test payment without hops
  

  
  //test more payments from same source
  sender = 0;
  receiver = 4;
  amount = 0.3;
  simulator_time = 0.0;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, simulator_time, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);
  
  sender = 0;
  receiver = 4;
  amount = 0.3;
  simulator_time = 0.0;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, simulator_time, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);

  sender = 0;
  receiver = 4;
  amount = 0.3;
  simulator_time = 0.0;
  payment = create_payment(payment_index, sender, receiver, amount);
  hash_table_put(payments, payment->id, payment);
  event = create_event(event_index, simulator_time, FINDROUTE, sender, payment->id);
  events = heap_insert(events, event, compare_event);
   //end test more payments from same source
   

  


 while(heap_len(events) != 0 ) {
    event = heap_pop(events, compare_event);

    switch(event->type){
    case FINDROUTE:
      find_route(event);
      break;
    case SENDPAYMENT:
      send_payment(event);
      break;
    case FORWARDPAYMENT:
      forward_payment(event);
      break;
    case RECEIVEPAYMENT:
      receive_payment(event);
      break;
    case FORWARDSUCCESS:
      forward_success(event);
      break;
    case RECEIVESUCCESS:
      receive_success(event);
      break;
    case FORWARDFAIL:
      forward_fail(event);
      break;
    case RECEIVEFAIL:
      receive_fail(event);
      break;
    default:
      printf("Error wrong event type\n");
    }
  }

  print_balances();


  return 0;
}

*/

/*Hash_table* peers, *channels, *channel_infos;
long n_peers, n_channels;
 
//test trasform_path_into_route
int main() {
  struct path_hop* path_hop;
  struct array *ignored;
  long i, sender, receiver;
  long fake_ignored = -1;
  struct route* route;
  struct array* route_hops, *path_hops;
  struct route_hop* route_hop;
  struct peer* peer;
  struct edge* channel;
  struct channel * channel_info;

  ignored = array_initialize(1);
  ignored = array_insert(ignored, &fake_ignored);

  peers = hash_table_initialize(2);
  channels = hash_table_initialize(2);
  channel_infos= hash_table_initialize(2);

  n_peers=5;

  for(i=0; i<n_peers; i++) {
      peer = create_peer(5);
      hash_table_put(peers, peer->id, peer);
    }

    for(i=1; i<5; i++) {
      connect_peers(i-1, i);
    }

    path_hops=dijkstra(0, 4, 1.0, ignored, ignored );
    route = transform_path_into_route(path_hops, 1.0, 5);
    printf("struct route/n");
    if(route==NULL) {
      printf("Null route/n");
      return 0;
    }

  for(i=0; i < array_len(route->route_hops); i++) {
    route_hop = array_get(route->route_hops, i);
    path_hop = route_hop->path_hop;
    printf("HOP %ld\n", i);
    printf("(Sender, Receiver, struct edge) = (%ld, %ld, %ld)\n", path_hop->sender, path_hop->receiver, path_hop->channel);
    printf("Amount to forward: %lf\n", route_hop->amount_to_forward);
    printf("Timelock: %d\n\n", route_hop->timelock);
  }

  printf("Total amount: %lf\n", route->total_amount);
  printf("Total fee: %lf\n", route->total_fee);
  printf("Total timelock: %d\n", route->total_timelock);

  return 0;
}
*/
/*
// Test Yen

Hash_table* peers, *channels, *channel_infos;
long n_peers, n_channels;

int main() {
  struct array *paths;
  struct array* path;
  struct path_hop* hop;
  long i, j;
  struct peer* peer;
  struct edge* channel;
  struct channel * channel_info;

  peers = hash_table_initialize(2);
  channels = hash_table_initialize(2);
  channel_infos= hash_table_initialize(2);

  n_peers=8;

  

  for(i=0; i<n_peers; i++) {
    peer = create_peer(5);
    hash_table_put(peers, peer->id, peer);
  }

  struct policy policy;
  policy.fee=0.0;
  policy.timelock=1.0;

  for(i=1; i<5; i++) {
    connect_peers(i-1, i);
  }

  connect_peers(0, 5);
  connect_peers(5,6);
  connect_peers(6,4);

  connect_peers(0,7);
  connect_peers(7,4);




  long *curr_channel_id;
  for(i=0; i<n_peers; i++) {
    peer = array_get(peers, i);
    //    printf("%ld ", array_len(peer->channel));
    for(j=0; j<array_len(peer->channel); j++) {
      curr_channel_id=array_get(peer->channel, j);
      if(curr_channel_id==NULL) {
        printf("null\n");
        continue;
      }
      channel = array_get(channels, *curr_channel_id);
      printf("struct peer %ld is connected to peer %ld through channel %ld\n", i, channel->counterparty, channel->id);
      }
  }



  printf("\n_yen\n");
  paths=find_paths(0, 4, 0.0);
  printf("%ld\n", array_len(paths));
  for(i=0; i<array_len(paths); i++) {
    printf("\n");
     path = array_get(paths, i);
     for(j=0;j<array_len(path); j++) {
       hop = array_get(path, j);
       printf("(Sender, Receiver, struct edge) = (%ld, %ld, %ld) ", hop->sender, hop->receiver, hop->channel);
     }
  }

  return 0;
}
*/


//test dijkstra
/* int main() { */
/*   struct array *hops; */
/*   struct path_hop* hop; */
/*   struct array *ignored; */
/*   long i, sender, receiver; */
/*   long fake_ignored = -1; */
/*   struct peer* peer[3]; */
/*   struct channel* channel_info; */
/*   struct edge* channel; */
/*   struct policy policy; */


/*   ignored = array_initialize(1); */
/*   ignored = array_insert(ignored, &fake_ignored); */

/*   peers = array_initialize(5); */
/*   channels = array_initialize(4); */
/*   channel_infos = array_initialize(2); */

/*   for(i=0; i<5; i++){ */
/*     peer[i] = create_peer_post_proc(i, 0); */
/*     peers = array_insert(peers, peer[i]);  */
/*   } */

/*   channel_info = create_channel_info_post_proc(0, 0, 1, 0, 1, 100, 0); */
/*   channel_infos = array_insert(channel_infos, channel_info); */
/*   channel_info = create_channel_info_post_proc(1, 2, 3, 1, 2, 100, 0); */
/*   channel_infos = array_insert(channel_infos, channel_info); */


/*   policy.fee_base = 1; */
/*   policy.fee_proportional =1; */
/*   policy.min_htlc = 0; */
/*   policy.timelock = 1; */

/*   channel = create_channel_post_proc(0, 0, 1, 1, 50, policy); */
/*   channels = array_insert(channels, channel); */
/*   peer[0]->channel = array_insert(peer[0]->channel, &channel->id); */

/*   channel = create_channel_post_proc(1, 0, 0, 0, 50, policy); */
/*   channels = array_insert(channels, channel); */
/*   peer[1]->channel = array_insert(peer[1]->channel, &channel->id); */

/*   channel = create_channel_post_proc(2, 1, 3, 2, 50, policy); */
/*   channels = array_insert(channels, channel); */
/*   peer[1]->channel = array_insert(peer[1]->channel, &channel->id); */

/*   channel = create_channel_post_proc(3, 1, 2, 1, 50, policy); */
/*   channels = array_insert(channels, channel); */
/*   peer[2]->channel = array_insert(peer[2]->channel, &channel->id); */


/*   initialize_dijkstra(); */
/*   printf("\n_dijkstra\n"); */

/*   sender = 2; */
/*   receiver = 0; */
/*   hops=dijkstra(sender, receiver, 10, ignored, ignored ); */
/*   printf("From node %ld to node %ld\n", sender, receiver); */
/*   if(hops!=NULL){ */
/*     for(i=0; i<array_len(hops); i++) { */
/*       hop = array_get(hops, i); */
/*       printf("(Sender, Receiver, struct edge) = (%ld, %ld, %ld) ", hop->sender, hop->receiver, hop->channel); */
/*     } */
/*   } */
/*   else  */
/*     printf("nix\n"); */

/*   /\* sender = 0; *\/ */
/*   /\* receiver = 1; *\/ */
/*   /\* hops=dijkstra(sender, receiver, 0.0, ignored, ignored ); *\/ */
/*   /\* printf("From node %ld to node %ld\n", sender, receiver); *\/ */
/*   /\* for(i=0; i<array_len(hops); i++) { *\/ */
/*   /\*   hop = array_get(hops, i); *\/ */
/*   /\*   printf("(Sender, Receiver, struct edge) = (%ld, %ld, %ld) ", hop->sender, hop->receiver, hop->channel); *\/ */
/*   /\* } *\/ */
/*   /\* printf("\n"); *\/ */

/*   /\* sender = 0; *\/ */
/*   /\* receiver = 0; *\/ */
/*   /\* printf("From node %ld to node %ld\n", sender, receiver); *\/ */
/*   /\* hops=dijkstra(sender,receiver, 0.0, ignored, ignored ); *\/ */
/*   /\* if(hops != NULL) { *\/ */
/*   /\*   for(i=0; i<array_len(hops); i++) { *\/ */
/*   /\*     hop = array_get(hops, i); *\/ */
/*   /\*     printf("(Sender, Receiver, struct edge) = (%ld, %ld, %ld) ", hop->sender, hop->receiver, hop->channel); *\/ */
/*   /\*   } *\/ */
/*   /\* } *\/ */
/*   /\* printf("\n"); *\/ */


/*   return 0; */
/* } */

/*
int main() {
  struct array *a;
  long i, *data;

  a=array_initialize(10);

  for(i=0; i< 21; i++){
    data = malloc(sizeof(long));
    *data = i;
   a = array_insert(a,data);
  }

  for(i=0; i<21; i++) {
    data = array_get(a,i);
    if(data!=NULL) printf("%ld ", *data);
    else printf("null ");
  }

  printf("\n%ld ", a->size);

  return 0;
}*/

/*
int main() {
  Hash_table *ht;
  long i;
  struct event *e;
  long N=100000;

  ht = initialize_hash_table(10);

  for(i=0; i<N; i++) {
    e = malloc(sizeof(struct event));
    e->time=0.0;
    e->id=i;
    strcpy(e->type, "Send");

    put(ht, e->id, e);

}

  long list_dim[10]={0};
  Element* el;
  for(i=0; i<10;i++) {
    el =ht->table[i];
    while(el!=NULL){
      list_dim[i]++;
      el = el->next;
    }
  }

  for(i=0; i<10; i++)
   printf("%ld ", list_dim[i]);

  printf("\n");

  return 0;
}


int main() {

  const gsl_rng_type * T;
  gsl_rng * r;

  int i, n = 10;
  double mu = 0.1;


  gsl_rng_env_setup();

  T = gsl_rng_default;
  r = gsl_rng_alloc (T);


  for (i = 0; i < n; i++)
    {
      double  k = gsl_ran_exponential (r, mu);
      printf (" %lf", k);
    }

  printf ("\n");
  gsl_rng_free (r);

  return 0;
   }
*/
