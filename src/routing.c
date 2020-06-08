#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include "../include/payments.h"
#include "../include/htlc.h"
#include "../include/heap.h"
#include "../include/array.h"
#include "../include/routing.h"
#include "../include/network.h"
#define INF UINT64_MAX
#define HOPSLIMIT 27
#define FINALTIMELOCK 40
#define TIMELOCKLIMIT 2016+FINALTIMELOCK
#define PROBABILITYLIMIT 0.01
#define RISKFACTOR 15
#define PAYMENTATTEMPTPENALTY 100000

struct distance **distance;
struct heap** distance_heap;
pthread_mutex_t data_mutex;
pthread_mutex_t jobs_mutex;
struct array** paths;
struct element* jobs=NULL;


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
    jobs = push(jobs, payment->id);
  }

}


void* dijkstra_thread(void*arg) {
  struct payment * payment;
  struct array* hops;
  long payment_id;
  struct thread_args *thread_args;
  enum pathfind_error error;

  thread_args = (struct thread_args*) arg;

  while(1) {
    pthread_mutex_lock(&jobs_mutex);
    jobs = pop(jobs, &payment_id);
    pthread_mutex_unlock(&jobs_mutex);

    if(payment_id==-1) return NULL;

    pthread_mutex_lock(&data_mutex);
    payment = array_get(thread_args->payments, payment_id);
    pthread_mutex_unlock(&data_mutex);

    hops = dijkstra(payment->sender, payment->receiver, payment->amount, thread_args->network, thread_args->data_index, &error);

    paths[payment->id] = hops;
  }

  return NULL;
}

void run_dijkstra_threads(struct network*  network, struct array* payments) {
  long i;
  pthread_t tid[N_THREADS];
  struct thread_args *thread_args;


  for(i=0; i<N_THREADS; i++) {
    thread_args = (struct thread_args*) malloc(sizeof(struct thread_args));
    thread_args->network = network;
    thread_args->payments = payments;
    thread_args->data_index = i;
    pthread_create(&(tid[i]), NULL, dijkstra_thread, (void*) thread_args);
   }

  for(i=0; i<N_THREADS; i++)
    pthread_join(tid[i], NULL);

}



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

int is_key_equal(struct distance* a, struct distance* b) {
  return a->node == b->node;
}


int is_ignored(long id, struct array* ignored){
  int i;
  struct ignored* curr;

  for(i=0; i<array_len(ignored); i++) {
    curr = array_get(ignored, i);
    if(curr->id==id)
      return 1;
  }

  return 0;
}

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
    from_node_id = edge->to_node_id; // search is performed from target to source
    if(is_in_list(explored_nodes, from_node_id))
      continue;
    explored_nodes = push(explored_nodes, from_node_id);

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
        if(edge->balance < amount)
          continue;
        if(amount < edge->policy.min_htlc)
          continue;
        if(edge->balance < max_balance)
          continue;
        max_balance = edge->balance;
        best_edge = edge;
      }
      else {
        if(amount > channel->capacity)
          continue;
        if(amount < edge->policy.min_htlc)
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

  return best_edges;
}


double get_edge_weight(uint64_t amount, uint64_t fee, uint32_t timelock){
  double timelock_penalty;
  timelock_penalty = amount*((double)timelock)*RISKFACTOR/((double)1000000000);
  return timelock_penalty + ((double) fee);
}


uint64_t get_probability_based_dist(double weight, double probability){
  const double min_probability = 0.00001;
  if(probability < min_probability)
    return INF;
  return weight + ((double) PAYMENTATTEMPTPENALTY)/probability;
}


struct array* dijkstra(long source, long target, uint64_t amount, struct network* network, long p, enum pathfind_error *error) {
  struct distance *d=NULL, to_node_dist;
  long i, best_node_id, j, from_node_id, curr;
  struct node *source_node;
  struct edge* edge=NULL;
  uint32_t edge_timelock, tmp_timelock;
  uint64_t  amt_to_send, edge_fee, tmp_dist, amt_to_receive, total_balance, max_balance, current_dist;
  struct array* hops=NULL, *best_edges = NULL;
  struct path_hop* hop=NULL;
  double edge_probability, tmp_probability, edge_weight, tmp_weight, current_prob;

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

    best_edges = get_best_edges(best_node_id, amt_to_send, source, network);

    for(j=0; j<array_len(best_edges); j++) {
      edge = array_get(best_edges, j);

      from_node_id = edge->from_node_id;

      //TODO: implement get_probability
      edge_probability = 1;
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

  return hops;
}


struct route* route_initialize(long n_hops) {
  struct route* r;

  r = malloc(sizeof(struct route));
  r->route_hops = array_initialize(n_hops);
  r->total_amount = 0;
  r->total_timelock = 0;

  return r;
}


//slightly differet w.r.t. `newRoute` in lnd because `newRoute` aims to produce the payloads for each node from the second in the path to the last node
struct route* transform_path_into_route(struct array* path_hops, uint64_t destination_amt, struct network* network) {
  struct path_hop *path_hop;
  struct route_hop *route_hop, *next_route_hop;
  struct route *route;
  long n_hops, i;
  uint64_t fee;
  struct edge* edge;
  struct policy current_edge_policy, next_edge_policy;

  n_hops = array_len(path_hops);
  if(n_hops > HOPSLIMIT)
    return NULL;

  route = route_initialize(n_hops);

  for(i=n_hops-1; i>=0; i--) {
    path_hop = array_get(path_hops, i);

    edge = array_get(network->edges, path_hop->edge);
    current_edge_policy = edge->policy;

    route_hop = malloc(sizeof(struct route_hop));
    route_hop->path_hop = path_hop;
    if(i == n_hops-1) {
      route_hop->amount_to_forward = destination_amt;
      fee = 0;
      route->total_amount += destination_amt;

      route_hop->timelock = FINALTIMELOCK;
      route->total_timelock += FINALTIMELOCK;
    }
    else {
      fee = compute_fee(next_route_hop->amount_to_forward, next_edge_policy);
      route_hop->amount_to_forward = next_route_hop->amount_to_forward + fee;
      route->total_amount += fee;
      route->total_fee += fee;

      route_hop->timelock = next_route_hop->timelock + current_edge_policy.timelock;
      route->total_timelock += current_edge_policy.timelock;
    }
    route->route_hops = array_insert(route->route_hops, route_hop);

    next_edge_policy = current_edge_policy;
    next_route_hop = route_hop;
    }

  array_reverse(route->route_hops);

  return route;
}


void print_hop(struct route_hop* hop){
  printf("Sender %ld, Receiver %ld, Edge %ld\n", hop->path_hop->sender, hop->path_hop->receiver, hop->path_hop->edge);
}


/* int is_same_path(struct array*root_path, struct array*path) { */
/*   long i; */
/*   struct path_hop* hop1, *hop2; */

/*   for(i=0; i<array_len(root_path); i++) { */
/*     hop1=array_get(root_path, i); */
/*     hop2=array_get(path, i); */
/*     if(hop1->edge != hop2->edge) */
/*       return 0; */
/*   } */

/*   return 1; */
/* } */


/* int compare_path(struct array* path1, struct array* path2){ */
/*   long len1, len2; */
/*   len1 = array_len(path1); */
/*   len2=array_len(path2); */

/*   if(len1==len2) return 0; */
/*   else if (len1<len2) return -1; */
/*   else return 1; */
/* } */

//BASIC DIJKSTRA - NOT THREADS
/* struct array* dijkstra(long source, long target, uint64_t amount, struct array* ignored_nodes, struct array* ignored_edges) { */
/*   struct distance *d=NULL, to_node_dist; */
/*   long i, best_node_id, j,edge_id, *other_edge_id=NULL, prev_node_id, curr; */
/*   struct node* best_node=NULL; */
/*   struct edge* edge=NULL, *other_edge=NULL; */
/*   struct channel* channel=NULL; */
/*   uint64_t capacity, amt_to_send, fee, tmp_dist, weight, new_amt_to_receive; */
/*   struct array* hops=NULL; */
/*   struct path_hop* hop=NULL; */

/*   printf("DIJKSTRA\n"); */

/*   while(heap_len(distance_heap[0])!=0) */
/*     heap_pop(distance_heap[0], compare_distance); */

/*   for(i=0; i<node_index; i++){ */
/*     distance[0][i].node = i; */
/*     distance[0][i].distance = INF; */
/*     distance[0][i].fee = 0; */
/*     distance[0][i].amt_to_receive = 0; */
/*     next_node[0][i].edge = -1; */
/*     next_node[0][i].node = -1; */
/*   } */

/*   distance[0][target].node = target; */
/*   distance[0][target].amt_to_receive = amount; */
/*   distance[0][target].fee = 0; */
/*   distance[0][target].distance = 0; */


/*   distance_heap[0] =  heap_insert(distance_heap[0], &distance[0][target], compare_distance); */


/*   while(heap_len(distance_heap[0])!=0) { */
/*     d = heap_pop(distance_heap[0], compare_distance); */
/*     best_node_id = d->node; */
/*     if(best_node_id==source) break; */

/*     best_node = array_get(nodes, best_node_id); */

/*     for(j=0; j<array_len(best_node->open_edges); j++) { */
/*       // need to get other direction of the edge as search is performed from target to source */
/*       other_edge_id = array_get(best_node->open_edges, j); */
/*       if(other_edge_id==NULL) continue; */
/*       other_edge = array_get(edges, *other_edge_id); */
/*       edge = array_get(edges, other_edge->other_edge_id); */

/*       prev_node_id = other_edge->counterparty; */
/*       edge_id = edge->id; */

/*       if(is_ignored(prev_node_id, ignored_nodes)) continue; */
/*       if(is_ignored(edge_id, ignored_edges)) continue; */

/*       to_node_dist = distance[0][best_node_id]; */
/*       amt_to_send = to_node_dist.amt_to_receive; */

/*       if(prev_node_id==source) */
/*         capacity = edge->balance; */
/*       else{ */
/*         channel = array_get(channels, edge->channel_id); */
/*         capacity = channel->capacity; */
/*       } */

/*       if(amt_to_send > capacity || amt_to_send < edge->policy.min_htlc) continue; */

/*       if(prev_node_id==source) */
/*         fee = 0; */
/*       else */
/*         fee = compute_fee(amt_to_send, edge->policy); */

/*       new_amt_to_receive = amt_to_send + fee; */

/*       weight = fee + new_amt_to_receive*edge->policy.timelock*15/1000000000; */

/*       tmp_dist = weight + to_node_dist.distance; */


/*       if(tmp_dist < distance[0][prev_node_id].distance) { */
/*         distance[0][prev_node_id].node = prev_node_id; */
/*         distance[0][prev_node_id].distance = tmp_dist; */
/*         distance[0][prev_node_id].amt_to_receive = new_amt_to_receive; */
/*         distance[0][prev_node_id].fee = fee; */

/*         next_node[0][prev_node_id].edge = edge_id; */
/*         next_node[0][prev_node_id].node = best_node_id; */

/*         distance_heap[0] = heap_insert(distance_heap[0], &distance[0][prev_node_id], compare_distance); */
/*       } */
/*       } */

/*     } */


/*   if(next_node[0][source].node == -1) { */
/*     return NULL; */
/*   } */


/*   hops = array_initialize(HOPSLIMIT); */
/*   curr = source; */
/*   while(curr!=target) { */
/*     hop = malloc(sizeof(struct path_hop)); */
/*     hop->sender = curr; */
/*     hop->edge = next_node[0][curr].edge; */
/*     hop->receiver = next_node[0][curr].node; */
/*     hops=array_insert(hops, hop); */

/*     curr = next_node[0][curr].node; */
/*   } */


/*   if(array_len(hops)>HOPSLIMIT) { */
/*     return NULL; */
/*   } */


/*   return hops; */
/* } */


/* version for not all peers (doesn't work)
void floyd_warshall() {
  long i, j, k, p;
  struct channel* channel_info;
  struct edge* direction1, *direction2;
  struct path_hop *hop1, *hop2;
  uint16_t* d, *newd, dij, dik, dkj;
  struct payment* payment;


  distht = hash_table_initialize(peer_index);
  nextht = hash_table_initialize(peer_index);

  for(i=0; i<channel_info_index; i++) {
    channel_info = hash_table_get(channel_infos, i);
    direction1 = hash_table_get(channels, channel_info->channel_direction1);
    direction2 = hash_table_get(channels, channel_info->channel_direction2);

    hash_table_matrix_put(distht, channel_info->peer1, channel_info->peer2, &(direction1->policy.timelock));
    hash_table_matrix_put(distht, channel_info->peer2, channel_info->peer1, &(direction2->policy.timelock));

    hop1 = malloc(sizeof(struct path_hop));
    hop1->sender = channel_info->peer1;
    hop1->receiver = channel_info->peer2;
    hop1->channel = channel_info->channel_direction1;
    hash_table_matrix_put(nextht, channel_info->peer1, channel_info->peer2, hop1);

    hop2 = malloc(sizeof(struct path_hop));
    hop2->sender = channel_info->peer2;
    hop2->receiver = channel_info->peer1;
    hop2->channel = channel_info->channel_direction2;
    hash_table_matrix_put(nextht, channel_info->peer2, channel_info->peer1, hop2);
  }

  long* peers_pay;
  peers_pay = malloc(sizeof(long)*2*payment_index);
  for(i=0; i<payment_index; i++) {
    payment = hash_table_get(payments, i);
    j = i*2;
    peers_pay[j] = payment->sender;
    peers_pay[j+1] = payment->receiver;
  }


  for(k=0; k<peer_index; k++) {
    for(p=0; p<payment_index*2; p++){
      i = peers_pay[p];
      for(j=0; j<peer_index; j++) {

        d = hash_table_matrix_get(distht, i, j);
        if(d==NULL && i!=j)
          dij = INF;
        else if (d==NULL && i==j)
          dij = 0;
        else 
          dij = *d;

        d = hash_table_matrix_get(distht, i, k);
        if(d==NULL && i!=k)
          dik = INF;
        else if (d==NULL && i==k)
          dik = 0;
        else{
          dik = *d;
          //  printf("%u\n", dik);
        }

        d = hash_table_matrix_get(distht, k, j);
        if(d==NULL && k!=j)
          dkj = INF;
        else if (d==NULL && k==j)
          dkj = 0;
        else{
          dkj = *d;
          //        printf("%u\n", dkj);
    }



        if(dij > dik + dkj) {
          newd = malloc(sizeof(uint16_t));
          *newd = dik+dkj;
          hash_table_matrix_put(distht, i, j, newd);
          hash_table_matrix_put(nextht, i, j, hash_table_matrix_get(nextht, i, k));
        }

      }
    }
    printf("%ld\n", k);
    }

}


struct array* get_path(long source, long destination) {
  struct array* path;
  long next_peer;
  struct path_hop* hop;

  hop = hash_table_matrix_get(nextht, source, destination);

  if(hop == NULL) {
    return NULL;
  }

  path = array_initialize(10);

  path = array_insert(path, hop);
  next_peer = hop->receiver;
  while(next_peer != destination ) {
    hop = hash_table_matrix_get(nextht, next_peer, destination);
    path = array_insert(path, hop);
    next_peer = hop->receiver;
  }

  if(array_len(path)>HOPSLIMIT)
    return NULL;

  return path;
}
*/

/*
void floyd_warshall() {
  long i, j, k;
  struct channel* channel_info;
  struct edge* direction1, *direction2;

  dist = malloc(sizeof(uint32_t*)*peer_index);
  next = malloc(sizeof(struct path_hop*)*peer_index);
  //  paths = malloc(sizeof(struct array**)*peer_index);
  for(i=0; i<peer_index; i++) {
    dist[i] = malloc(sizeof(uint32_t)*peer_index);
    next[i] = malloc(sizeof(struct path_hop)*peer_index);
    //paths[i] = malloc(sizeof(struct array*)*peer_index);
  }


  for(i=0; i<peer_index; i++){
    for(j=0; j<peer_index; j++) {
      if(i==j)
        dist[i][j] = 0;
      else
        dist[i][j] = INF;

      next[i][j].channel = -1;
      //      paths[i][j] = array_initialize(10);
    }
  }

  for(i=0; i<channel_info_index; i++) {
    channel_info = hash_table_get(channel_infos, i);
    direction1 = hash_table_get(channels, channel_info->channel_direction1);
    direction2 = hash_table_get(channels, channel_info->channel_direction2);
    dist[channel_info->peer1][channel_info->peer2] = direction1->policy.timelock;
    dist[channel_info->peer2][channel_info->peer1] = direction2->policy.timelock;
    next[channel_info->peer1][channel_info->peer2].sender = channel_info->peer1;
    next[channel_info->peer1][channel_info->peer2].receiver = channel_info->peer2;
    next[channel_info->peer1][channel_info->peer2].channel = channel_info->channel_direction1;
    next[channel_info->peer2][channel_info->peer1].sender = channel_info->peer2;
    next[channel_info->peer2][channel_info->peer1].receiver = channel_info->peer1;
    next[channel_info->peer2][channel_info->peer1].channel = channel_info->channel_direction2;
  }

  for(k=0; k<peer_index; k++) {
    for(i=0; i<peer_index; i++){
      for(j=0; j<peer_index; j++){
        if(dist[i][j] > dist[i][k] + dist[k][j]) {
          dist[i][j] = dist[i][k] + dist[k][j];
          next[i][j] = next[i][k];
        }
      }
    }
  }

  //to be commented
  for(i=0; i<payment_index; i++) {
    payment = hash_table_get(payments, i);
    source = payment->sender;
    destination = payment->receiver;

    if(next[source][destination].channel==-1)
      continue;

    paths[source][destination] = array_insert(paths[source][destination], &(next[source][destination]));
    next_peer = next[source][destination].receiver;
    while(next_peer != destination ) {
      paths[source][destination] = array_insert(paths[source][destination], &(next[next_peer][destination]));
      next_peer = next[next_peer][destination].receiver;
    }

    if(array_len(paths[source][destination])>HOPSLIMIT)
      array_delete_all(paths[source][destination]);
  }
// end to-be-commented

}

struct array* get_path(long source, long destination) {
  struct array* path;
  long next_peer;

  if(next[source][destination].channel==-1) {
    return NULL;
  }
  path = array_initialize(10);

  path = array_insert(path, &(next[source][destination]));
  next_peer = next[source][destination].receiver;
  while(next_peer != destination ) {
    path = array_insert(path, &(next[next_peer][destination]));
    next_peer = next[next_peer][destination].receiver;
  }

  if(array_len(path)>HOPSLIMIT)
    return NULL;

  return path;
}
*/

// OLD ROUTING VERSIONS (before lnd-0.5-beta)

/* struct array* dijkstra_p(long source, long target, uint64_t amount, struct array* ignored_peers, struct array* ignored_channels, long p) { */
/*   struct distance *d=NULL; */
/*   long i, best_peer_id, j,*channel_id=NULL, next_peer_id, prev; */
/*   struct peer* best_peer=NULL; */
/*   struct edge* channel=NULL; */
/*   struct channel* channel_info=NULL; */
/*   uint32_t tmp_dist; */
/*   uint64_t capacity; */
/*   struct path_hop* hop=NULL; */
/*   struct array* hops=NULL; */

/*   while(heap_len(distance_heap[p])!=0) { */
/*     heap_pop(distance_heap[p], compare_distance); */
/*   } */


/*   for(i=0; i<peer_index; i++){ */
/*     distance[p][i].peer = i; */
/*     distance[p][i].distance = INF; */
/*     previous_peer[p][i].channel = -1; */
/*     previous_peer[p][i].peer = -1; */
/*   } */

/*   distance[p][source].peer = source; */
/*   distance[p][source].distance = 0; */

/*   distance_heap[p] =  heap_insert(distance_heap[p], &distance[p][source], compare_distance); */

/*   i=0; */
/*   while(heap_len(distance_heap[p])!=0) { */
/*     i++; */
/*     d = heap_pop(distance_heap[p], compare_distance); */

/*     best_peer_id = d->peer; */
/*     if(best_peer_id==target) break; */

/*     best_peer = array_get(peers, best_peer_id); */

/*     for(j=0; j<array_len(best_peer->channel); j++) { */
/*       channel_id = array_get(best_peer->channel, j); */
/*       if(channel_id==NULL) continue; */

/*       channel = array_get(channels, *channel_id); */

/*       next_peer_id = channel->counterparty; */

/*       if(is_present(next_peer_id, ignored_peers)) continue; */
/*       if(is_present(*channel_id, ignored_channels)) continue; */

/*       tmp_dist = distance[p][best_peer_id].distance + channel->policy.timelock; */

/*       channel_info = array_get(channel_infos, channel->channel_info_id); */

/*       capacity = channel_info->capacity; */

/*       if(tmp_dist < distance[p][next_peer_id].distance && amount<=capacity && amount >= channel->policy.min_htlc) { */
/*         distance[p][next_peer_id].peer = next_peer_id; */
/*         distance[p][next_peer_id].distance = tmp_dist; */

/*         previous_peer[p][next_peer_id].channel = *channel_id; */
/*         previous_peer[p][next_peer_id].peer = best_peer_id; */

/*         distance_heap[p] = heap_insert(distance_heap[p], &distance[p][next_peer_id], compare_distance); */
/*       } */
/*       } */

/*     } */



/*   if(previous_peer[p][target].peer == -1) { */
/*     return NULL; */
/*   } */


/*   i=0; */
/*   hops=array_initialize(HOPSLIMIT); */
/*   prev=target; */
/*   while(prev!=source) { */
/*     channel = array_get(channels, previous_peer[p][prev].channel); */

/*     hop = malloc(sizeof(struct path_hop)); */

/*     hop->channel = previous_peer[p][prev].channel; */
/*     hop->sender = previous_peer[p][prev].peer; */
/*     hop->receiver = channel->counterparty; */

/*     hops=array_insert(hops, hop ); */

/*     prev = previous_peer[p][prev].peer; */

/*   } */


/*   if(array_len(hops)>HOPSLIMIT) { */
/*     return NULL; */
/*   } */

/*   array_reverse(hops); */


/*   return hops; */
/* } */


/* struct array* dijkstra(long source, long target, uint64_t amount, struct array* ignored_peers, struct array* ignored_channels) { */
/*   struct distance *d=NULL; */
/*   long i, best_peer_id, j,*channel_id=NULL, next_peer_id, prev; */
/*   struct peer* best_peer=NULL; */
/*   struct edge* channel=NULL; */
/*   struct channel* channel_info=NULL; */
/*   uint32_t tmp_dist; */
/*   uint64_t capacity; */
/*   struct array* hops=NULL; */
/*   struct path_hop* hop=NULL; */

/*   printf("DIJKSTRA\n"); */

/*   while(heap_len(distance_heap[0])!=0) */
/*     heap_pop(distance_heap[0], compare_distance); */

/*   for(i=0; i<peer_index; i++){ */
/*     distance[0][i].peer = i; */
/*     distance[0][i].distance = INF; */
/*     previous_peer[0][i].channel = -1; */
/*     previous_peer[0][i].peer = -1; */
/*   } */

/*   distance[0][source].peer = source; */
/*   distance[0][source].distance = 0; */

/*   distance_heap[0] =  heap_insert(distance_heap[0], &distance[0][source], compare_distance); */

/*   while(heap_len(distance_heap[0])!=0) { */
/*     d = heap_pop(distance_heap[0], compare_distance); */
/*     best_peer_id = d->peer; */
/*     if(best_peer_id==target) break; */

/*     best_peer = array_get(peers, best_peer_id); */

/*     for(j=0; j<array_len(best_peer->channel); j++) { */
/*       channel_id = array_get(best_peer->channel, j); */
/*       if(channel_id==NULL) continue; */

/*       channel = array_get(channels, *channel_id); */

/*       next_peer_id = channel->counterparty; */

/*       if(is_present(next_peer_id, ignored_peers)) continue; */
/*       if(is_present(*channel_id, ignored_channels)) continue; */

/*       tmp_dist = distance[0][best_peer_id].distance + channel->policy.timelock; */

/*       channel_info = array_get(channel_infos, channel->channel_info_id); */

/*       capacity = channel_info->capacity; */

/*       if(tmp_dist < distance[0][next_peer_id].distance && amount<=capacity) { */
/*         distance[0][next_peer_id].peer = next_peer_id; */
/*         distance[0][next_peer_id].distance = tmp_dist; */

/*         previous_peer[0][next_peer_id].channel = *channel_id; */
/*         previous_peer[0][next_peer_id].peer = best_peer_id; */

/*         distance_heap[0] = heap_insert(distance_heap[0], &distance[0][next_peer_id], compare_distance); */
/*       } */
/*       } */

/*     } */


/*   if(previous_peer[0][target].peer == -1) { */
/*     return NULL; */
/*   } */


/*   hops=array_initialize(HOPSLIMIT); */
/*   prev=target; */
/*   while(prev!=source) { */
/*     hop = malloc(sizeof(struct path_hop)); */
/*     hop->channel = previous_peer[0][prev].channel; */
/*     hop->sender = previous_peer[0][prev].peer; */

/*    channel = array_get(channels, hop->channel); */

/*     hop->receiver = channel->counterparty; */
/*     hops=array_insert(hops, hop ); */
/*     prev = previous_peer[0][prev].peer; */
/*   } */


/*   if(array_len(hops)>HOPSLIMIT) { */
/*     return NULL; */
/*   } */

/*   array_reverse(hops); */

/*   return hops; */
/* } */



/* struct array* find_paths(long source, long target, double amount){ */
/*   struct array* starting_path, *first_path, *prev_shortest, *root_path, *path, *spur_path, *new_path, *next_shortest_path; */
/*   struct array* ignored_channels, *ignored_peers; */
/*  struct path_hop *hop; */
/*   long i, k, j, spur_node, new_path_len; */
/*   struct array* shortest_paths; */
/*   struct heap* candidate_paths; */

/*   candidate_paths = heap_initialize(100); */

/*   ignored_peers=array_initialize(2); */
/*   ignored_channels=array_initialize(2); */

/*   shortest_paths = array_initialize(100); */

/*   starting_path=dijkstra(source, target, amount, ignored_peers, ignored_channels); */
/*   if(starting_path==NULL) return NULL; */

/*   first_path = array_initialize(array_len(starting_path)+1); */
/*   hop = malloc(sizeof(struct path_hop)); */
/*   hop->receiver = source; */
/*   first_path=array_insert(first_path, hop); */
/*   for(i=0; i<array_len(starting_path); i++) { */
/*     hop = array_get(starting_path, i); */
/*     first_path=array_insert(first_path, hop); */
/*   } */

/*   shortest_paths = array_insert(shortest_paths, first_path); */

/*   for(k=1; k<100; k++) { */
/*     prev_shortest = array_get(shortest_paths, k-1); */

/*     for(i=0; i<array_len(prev_shortest)-1; i++) { */
/*       hop = array_get(prev_shortest, i); */
/*       spur_node = hop->receiver; */

/*       root_path = array_initialize(i+1); */
/*       for(j=0; j<i+1; j++) { */
/*         hop = array_get(prev_shortest, j); */
/*         root_path=array_insert(root_path, hop); */
/*       } */

/*       for(j=0; j<array_len(shortest_paths); j++) { */
/*         path=array_get(shortest_paths, j); */
/*         if(array_len(path)>i+1) { */
/*           if(is_same_path(root_path, path)) { */
/*             hop = array_get(path, i+1); */
/*             ignored_channels = array_insert(ignored_channels, &(hop->channel)); */
/*           } */
/*         } */
/*       } */

/*       for(j=0; j<array_len(root_path); j++) { */
/*         hop = array_get(root_path, j); */
/*         if(hop->receiver == spur_node) continue; */
/*         ignored_peers = array_insert(ignored_peers, &(hop->receiver)); */
/*       } */

/*       spur_path = dijkstra(spur_node, target, amount, ignored_peers, ignored_channels); */

/*       if(spur_path==NULL) { */
/*         if(strcmp(error, "no_path")==0) continue; */
/*         else return NULL; */
/*       } */

/*       new_path_len = array_len(root_path) + array_len(spur_path); */
/*       new_path = array_initialize(new_path_len); */
/*       for(j=0; j<array_len(root_path); j++) { */
/*         hop = array_get(root_path, j); */
/*         new_path = array_insert(new_path, hop); */
/*       } */
/*       for(j=0; j<array_len(spur_path); j++) { */
/*         hop = array_get(spur_path, j); */
/*         new_path = array_insert(new_path, hop); */
/*       } */

/*       heap_insert(candidate_paths, new_path, compare_path); */
/*     } */

/*     if(heap_len(candidate_paths)==0) break; */

/*     next_shortest_path = heap_pop(candidate_paths, compare_path); */
/*     shortest_paths = array_insert(shortest_paths, next_shortest_path); */
/*   } */

/*   return shortest_paths; */

/* } */
