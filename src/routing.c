#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <math.h>
#include "../include/payments.h"
#include "../include/htlc.h"
#include "../include/heap.h"
#include "../include/array.h"
#include "../include/routing.h"
#include "../include/network.h"
#include "../include/utils.h"

/* Functions in this file simulate the path finding implemented in Lightning Network to find a path between the payment sender and the payment receiver.
   They are a (high-level) copy of functions lnd-v0.10.0-beta (see files `routing/pathfind.go`, `routing/payment_session.go` */

#define INF UINT64_MAX
#define HOPSLIMIT 27
#define TIMELOCKLIMIT 2016+FINALTIMELOCK
#define PROBABILITYLIMIT 0.01
#define RISKFACTOR 15
#define PAYMENTATTEMPTPENALTY 100000
#define APRIORIWEIGHT 0.5
#define APRIORIHOPPROBABILITY 0.6
#define PREVSUCCESSPROBABILITY 0.95
#define PENALTYHALFLIFE 1
#define MAXMILLISATOSHI UINT64_MAX


struct distance **distance;
struct heap** distance_heap;
pthread_mutex_t data_mutex;
pthread_mutex_t jobs_mutex;
struct array** paths;
struct element* jobs=NULL;


/* intialize the data structures of dijkstra and the jobs to be executed by the dijkstra threads */
void initialize_dijkstra(long n_nodes, long n_edges, struct array* payments) {
  int i;
  struct payment *payment;

  distance = malloc(sizeof(struct distance*)*N_THREADS);
  distance_heap = malloc(sizeof(struct heap*)*N_THREADS);
  for(i=0; i<N_THREADS; i++) {
    distance[i] = malloc(sizeof(struct distance)*n_nodes);
    distance_heap[i] = heap_initialize(n_edges);
  }

  pthread_mutex_init(&data_mutex, NULL);
  pthread_mutex_init(&jobs_mutex, NULL);

  paths = malloc(sizeof(struct array*)*array_len(payments));
  for(i=0; i<array_len(payments) ;i++){
    paths[i] = NULL;
    payment = array_get(payments, i);
    jobs = push(jobs, &(payment->id));
  }

}

/* a dijkstra thread finds a path for a payment by calling dijkstra */ 
void* dijkstra_thread(void*arg) {
  struct payment * payment;
  struct array* hops;
  void *data;
  long payment_id;
  struct thread_args *thread_args;
  enum pathfind_error error;
  thread_args = (struct thread_args*) arg;

  while(1) {
    if(jobs == NULL) return NULL;
    pthread_mutex_lock(&jobs_mutex);
    jobs = pop(jobs, &data);
    payment_id =  *((long*)data);
    pthread_mutex_unlock(&jobs_mutex);
    pthread_mutex_lock(&data_mutex);
    payment = array_get(thread_args->payments, payment_id);
    pthread_mutex_unlock(&data_mutex);
    hops = dijkstra(payment->sender, payment->receiver, payment->amount, thread_args->network, thread_args->current_time, thread_args->data_index, &error);
    paths[payment->id] = hops;
  }

  return NULL;
}


/* run dijkstra threads to find the initial paths of the payments (before the simulation starts) */
void run_dijkstra_threads(struct network*  network, struct array* payments, uint64_t current_time) {
  long i;
  pthread_t tid[N_THREADS];
  struct thread_args *thread_args;

  for(i=0; i<N_THREADS; i++) {
    thread_args = (struct thread_args*) malloc(sizeof(struct thread_args));
    thread_args->network = network;
    thread_args->payments = payments;
    thread_args->current_time = current_time;
    thread_args->data_index = i;
    pthread_create(&(tid[i]), NULL, dijkstra_thread, (void*) thread_args);
   }

  for(i=0; i<N_THREADS; i++)
    pthread_join(tid[i], NULL);
}


/* BEGIN - PROBABILITY FUNCTIONS */
/* these functions are used in dijkstra to calculate the probability that a payment will be successfully forwarded in an edge;
   this probability depends on the results of the previous payments performed by the sender node (see node pair result in htlc.c) */

double millisec_to_hour(double time){
  double res;
  res = time / 1000.0;
  res = res / 60.0;
  res = res / 24.0;
  return res;
}


double get_weight(double age){
  double exp;
  exp = - millisec_to_hour(age) / PENALTYHALFLIFE;
  return pow(2, exp);
}

double calculate_probability(struct element* node_results, long to_node_id, uint64_t amount, double node_probability, uint64_t current_time){
  struct node_pair_result* result;
  uint64_t time_since_last_failure;
  double weight, probability;

  result = get_by_key(node_results, to_node_id, is_equal_key_result);

  if(result == NULL)
    return node_probability;

  if(amount <= result->success_amount)
    return PREVSUCCESSPROBABILITY;

  if(result->fail_time == 0 || amount < result->fail_amount)
    return node_probability;

  if(result->fail_time > current_time) {
    fprintf(stderr, "ERROR (calculate_probability): fail_time > current_time" );
    exit(-1);
  }
  time_since_last_failure = current_time - result->fail_time;
  weight = get_weight((double)time_since_last_failure);
  probability = node_probability * (1-weight);

  return probability;
}


double get_node_probability(struct element* node_results, uint64_t amount, uint64_t current_time){
  double apriori_factor, total_probabilities, total_weight;
  struct element* iterator;
  struct node_pair_result* result;
  uint64_t age;

  if(list_len(node_results) == 0)
    return APRIORIHOPPROBABILITY;

  apriori_factor = 1.0 / (1.0 - APRIORIWEIGHT) - 1;
  total_probabilities = APRIORIHOPPROBABILITY*apriori_factor;
  total_weight = apriori_factor;
  for(iterator = node_results; iterator != NULL; iterator = iterator->next){
    result = (struct node_pair_result*) iterator->data;
    if(amount <= result->success_amount){
      total_weight++;
      total_probabilities += PREVSUCCESSPROBABILITY;
      continue;
    }
    if(result->fail_time != 0 && amount >= result->fail_amount){
      age = current_time - result->fail_time;
      total_weight += get_weight((double)age);
    }
  }

  return total_probabilities / total_weight;
}


double get_probability(long from_node_id, long to_node_id, uint64_t amount, long sender_id, uint64_t current_time,  struct network* network){
  struct node* sender;
  struct element* results;
  double node_probability;

  sender = array_get(network->nodes, sender_id);
  results = sender->results[from_node_id];

  if(from_node_id == sender_id)
    node_probability = PREVSUCCESSPROBABILITY;
  else
    node_probability = get_node_probability(results, amount, current_time);

  return calculate_probability(results, to_node_id, MAXMILLISATOSHI, node_probability, current_time);
}


uint64_t get_probability_based_dist(double weight, double probability){
  const double min_probability = 0.00001;
  if(probability < min_probability)
    return INF;
  return weight + ((double) PAYMENTATTEMPTPENALTY)/probability;
}


/* END - PROBABILITY FUNCTIONS */

/* compare the distance used in dijkstra */
int compare_distance(struct distance* a, struct distance* b) {
  uint64_t d1, d2;
  double p1, p2;
  d1=a->distance;
  d2=b->distance;
  p1=a->probability;
  p2=b->probability;
  if(d1==d2){
    if(p1>=p2)
      return 1;
    else
      return -1;
  }
  else if(d1<d2)
    return -1;
  else
    return 1;
}

/* get maximum and total balance of the edges of a node */
void get_balance(struct node* node, uint64_t *max_balance, uint64_t *total_balance){
  int i;
  struct edge* edge;

  *total_balance = 0;
  *max_balance = 0;
  for(i=0; i<array_len(node->open_edges); i++){
    edge = array_get(node->open_edges, i);
    *total_balance += edge->balance;
    if(edge->balance > *max_balance)
      *max_balance = edge->balance;
  }
}

/* get the best edges connecting to a node (in terms of low fees and low timelock) */
/* currently NOT USED because it requires large memory and computing resources */
struct array* get_best_edges(long to_node_id, uint64_t amount, long source_node_id, struct network* network){
  struct array* best_edges;
  struct element* explored_nodes = NULL;
  int i, j;
  struct node* to_node;
  struct edge* edge, *best_edge = NULL, *new_best_edge;
  struct channel* channel;
  long from_node_id, counter_edge_id;
  uint64_t max_balance, max_fee, fee;
  uint32_t max_timelock;
  unsigned int local_node;
  struct policy modified_policy;

  to_node = array_get(network->nodes, to_node_id);
  best_edges = array_initialize(5);

  for(i=0; i<array_len(to_node->open_edges); i++){
    edge = array_get(to_node->open_edges, i);
    if(is_in_list(explored_nodes, &(edge->to_node_id), is_equal_long))
      continue;
    explored_nodes = push(explored_nodes, &(edge->to_node_id));
    from_node_id = edge->to_node_id;//search is performed in reverse, from target to source

    max_balance = 0;
    max_fee = 0;
    max_timelock = 0;
    best_edge = NULL;
    local_node = source_node_id == from_node_id;
    for(j=0; j<array_len(to_node->open_edges); j++){
      edge = array_get(to_node->open_edges, j);
      if(edge->to_node_id != from_node_id)
        continue;
      counter_edge_id = edge->counter_edge_id;
      edge = array_get(network->edges, counter_edge_id);
      channel = array_get(network->channels, edge->channel_id);

      if(local_node){
        if(edge->balance < amount || amount < edge->policy.min_htlc || edge->balance < max_balance)
          continue;
        max_balance = edge->balance;
        best_edge = edge;
      }
      else {
        if(amount > channel->capacity || amount < edge->policy.min_htlc)
          continue;
        if(edge->policy.timelock > max_timelock)
          max_timelock = edge->policy.timelock;
        fee = compute_fee(amount, edge->policy);
        if(fee < max_fee)
          continue;
        max_fee = fee;
        best_edge = edge;
      }
    }

    if(best_edge == NULL)
      continue;
    if(!local_node){
      modified_policy = best_edge->policy;
      modified_policy.timelock = max_timelock;
      new_best_edge = new_edge(best_edge->id, best_edge->channel_id, best_edge->counter_edge_id, best_edge->from_node_id, best_edge->to_node_id, best_edge->balance, modified_policy);
    }
    else {
      new_best_edge = best_edge;
    }
    best_edges = array_insert(best_edges, new_best_edge);
  }

  list_free(explored_nodes);

  return best_edges;
}

/* get the weight of an edge which depends on the timelock and fee required in the edge */
double get_edge_weight(uint64_t amount, uint64_t fee, uint32_t timelock){
  double timelock_penalty;
  timelock_penalty = amount*((double)timelock)*RISKFACTOR/((double)1000000000);
  return timelock_penalty + ((double) fee);
}

/* a modified version of dijkstra to find a path connecting the source (payment sender) to the target (payment receiver) */
struct array* dijkstra(long source, long target, uint64_t amount, struct network* network, uint64_t current_time, long p, enum pathfind_error *error) {
  struct distance *d=NULL, to_node_dist;
  long i, best_node_id, j, from_node_id, curr;
  struct node *source_node, *best_node;
  struct edge* edge=NULL;
  uint64_t edge_timelock, tmp_timelock;
  uint64_t  amt_to_send, edge_fee, tmp_dist, amt_to_receive, total_balance, max_balance, current_dist;
  struct array* hops=NULL; // *best_edges = NULL;
  struct path_hop* hop=NULL;
  double edge_probability, tmp_probability, edge_weight, tmp_weight, current_prob;
  struct channel* channel;

  source_node = array_get(network->nodes, source);
  get_balance(source_node, &max_balance, &total_balance);
  if(amount > total_balance){
    *error = NOLOCALBALANCE;
    return NULL;
  }
  else if(amount > max_balance){
    *error = NOPATH;
    return NULL;
  }

  while(heap_len(distance_heap[p])!=0)
    heap_pop(distance_heap[p], compare_distance);

  for(i=0; i<array_len(network->nodes); i++){
    distance[p][i].node = i;
    distance[p][i].distance = INF;
    distance[p][i].fee = 0;
    distance[p][i].amt_to_receive = 0;
    distance[p][i].next_edge = -1;
  }

  distance[p][target].node = target;
  distance[p][target].amt_to_receive = amount;
  distance[p][target].fee = 0;
  distance[p][target].distance = 0;
  distance[p][target].timelock = FINALTIMELOCK;
  distance[p][target].weight = 0;
  distance[p][target].probability = 1;

  distance_heap[p] =  heap_insert_or_update(distance_heap[p], &distance[p][target], compare_distance, is_key_equal);

  while(heap_len(distance_heap[p])!=0) {

    d = heap_pop(distance_heap[p], compare_distance);
    best_node_id = d->node;
    if(best_node_id==source) break;

    to_node_dist = distance[p][best_node_id];
    amt_to_send = to_node_dist.amt_to_receive;

    best_node = array_get(network->nodes, best_node_id);
    /* best_edges = get_best_edges(best_node_id, amt_to_send, source, network); */

    for(j=0; j<array_len(best_node->open_edges); j++) {
      edge = array_get(best_node->open_edges, j);
      edge = array_get(network->edges, edge->counter_edge_id);

      from_node_id = edge->from_node_id;
      if(from_node_id == source){
        if(edge->balance < amt_to_send)
          continue;
      }
      else{
        channel = array_get(network->channels, edge->channel_id);
        if(channel->capacity < amt_to_send)
          continue;
      }

      if(amt_to_send < edge->policy.min_htlc)
        continue;

      edge_probability = get_probability(from_node_id, to_node_dist.node, amt_to_send, source, current_time, network);

      if(edge_probability == 0) continue;

      edge_fee = 0;
      edge_timelock = 0;
      if(from_node_id != source){
        edge_fee = compute_fee(amt_to_send, edge->policy);
        edge_timelock = edge->policy.timelock;
      }

      amt_to_receive = amt_to_send + edge_fee;

      tmp_timelock = to_node_dist.timelock + edge_timelock;
      if(tmp_timelock > TIMELOCKLIMIT) continue;

      tmp_probability = to_node_dist.probability*edge_probability;
      if(tmp_probability < PROBABILITYLIMIT) continue;

      edge_weight = get_edge_weight(amt_to_receive, edge_fee, edge_timelock);
      tmp_weight = to_node_dist.weight + edge_weight;
      tmp_dist = get_probability_based_dist(tmp_weight, tmp_probability);

      current_dist = distance[p][from_node_id].distance;
      current_prob = distance[p][from_node_id].probability;
      if(tmp_dist > current_dist) continue;
      if(tmp_dist == current_dist && tmp_probability <= current_prob) continue;

      distance[p][from_node_id].node = from_node_id;
      distance[p][from_node_id].distance = tmp_dist;
      distance[p][from_node_id].weight = tmp_weight;
      distance[p][from_node_id].amt_to_receive = amt_to_receive;
      distance[p][from_node_id].timelock = tmp_timelock;
      distance[p][from_node_id].probability = tmp_probability;
      distance[p][from_node_id].next_edge = edge->id;

      distance_heap[p] = heap_insert_or_update(distance_heap[p], &distance[p][from_node_id], compare_distance, is_key_equal);
    }
    }

  hops = array_initialize(5);
  curr = source;
  while(curr!=target) {
    if(distance[p][curr].next_edge == -1) {
      *error = NOPATH;
      return NULL;
    }
    hop = malloc(sizeof(struct path_hop));
    hop->sender = curr;
    hop->edge = distance[p][curr].next_edge;
    edge = array_get(network->edges, distance[p][curr].next_edge);
    hop->receiver = edge->to_node_id;
    hops=array_insert(hops, hop);
    curr = edge->to_node_id;
  }

  if(array_len(hops) > HOPSLIMIT){
    *error = NOPATH;
    return NULL;
  }

  return hops;
}


struct route* route_initialize(long n_hops) {
  struct route* r;
  r = malloc(sizeof(struct route));
  r->route_hops = array_initialize(n_hops);
  r->total_amount = 0;
  r->total_timelock = 0;
  r->total_fee = 0;
  return r;
}


/* transform a path into a route by computing fees and timelocks required at each hop in the path */
/* slightly differet w.r.t. `newRoute` in lnd because `newRoute` aims to produce the payloads for each node from the second in the path to the last node */
struct route* transform_path_into_route(struct array* path_hops, uint64_t destination_amt, struct network* network) {
  struct path_hop *path_hop;
  struct route_hop *route_hop, *next_route_hop;
  struct route *route;
  long n_hops, i;
  uint64_t fee;
  struct edge* edge;
  struct policy current_edge_policy, next_edge_policy;

  n_hops = array_len(path_hops);
  route = route_initialize(n_hops);

  for(i=n_hops-1; i>=0; i--) {
    path_hop = array_get(path_hops, i);

    edge = array_get(network->edges, path_hop->edge);
    current_edge_policy = edge->policy;

    route_hop = malloc(sizeof(struct route_hop));
    route_hop->from_node_id = path_hop->sender;
    route_hop->to_node_id = path_hop->receiver;
    route_hop->edge_id = path_hop->edge;
    if(i == n_hops-1) {
      route_hop->amount_to_forward = destination_amt;
      route->total_amount += destination_amt;
      //my version
      route_hop->timelock = FINALTIMELOCK;
      route->total_timelock += FINALTIMELOCK;
      //lnd version
      /* route->total_timelock += FINALTIMELOCK; */
      /* route_hop->timelock = route->total_timelock; */
    }
    else {
      fee = compute_fee(next_route_hop->amount_to_forward, next_edge_policy);
      route_hop->amount_to_forward = next_route_hop->amount_to_forward + fee;
      route->total_amount += fee;
      route->total_fee += fee;
      //my version
      route_hop->timelock = next_route_hop->timelock + current_edge_policy.timelock;
      route->total_timelock += current_edge_policy.timelock;
      //lnd version
      /* route_hop->timelock = route->total_timelock; */
      /* route->total_timelock += next_edge_policy.timelock; */
    }
    route->route_hops = array_insert(route->route_hops, route_hop);

    next_edge_policy = current_edge_policy;
    next_route_hop = route_hop;
     }

  array_reverse(route->route_hops);

  return route;
}
