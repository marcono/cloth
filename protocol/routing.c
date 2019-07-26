#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
//#include "../gc-7.2/include/gc.h"
#include "../simulator/initialize.h"
#include "../protocol/protocol.h"
#include "../utils/heap.h"
#include "../utils/array.h"
#include "routing.h"
#include "../global.h"
#define INF UINT64_MAX
#define HOPSLIMIT 20

char error[100];
uint32_t** dist;
Path_hop** next;



int present(long i) {
  long p;
  Payment *payment;

  for(p=0; p<payment_index; p++) {
    payment = array_get(payments, p);
    if(i==payment->sender || i == payment->receiver) return 1;
  }

  return 0;
}

int compare_distance(Distance* a, Distance* b) {
  uint64_t d1, d2;
  d1=a->distance;
  d2=b->distance;
  if(d1==d2)
    return 0;
  else if(d1<d2)
    return -1;
  else
    return 1;
}


Distance **distance;
Dijkstra_hop ** next_peer;
//Dijkstra_hop ** previous_peer;
Heap** distance_heap;
//Array** hops;

void initialize_dijkstra() {
  int i;

  distance = malloc(sizeof(Distance*)*PARALLEL);
  next_peer = malloc(sizeof(Dijkstra_hop*)*PARALLEL);
  distance_heap = malloc(sizeof(Heap*)*PARALLEL);

  for(i=0; i<PARALLEL; i++) {
    distance[i] = malloc(sizeof(Distance)*peer_index);
     next_peer[i] = malloc(sizeof(Dijkstra_hop)*peer_index);
    distance_heap[i] = heap_initialize(edge_index);
  }

}


int is_ignored(long ID, Array* ignored){
  int i;
  Ignored* curr;

  for(i=0; i<array_len(ignored); i++) {
    curr = array_get(ignored, i);
    if(curr->ID==ID)
      return 1;
  }

  return 0;
}

Array* dijkstra_p(long source, long target, uint64_t amount, Array* ignored_peers, Array* ignored_edges, long p) {
  Distance *d=NULL, to_node_dist;
  long i, best_peer_id, j,edge_id, *other_edge_id=NULL, prev_peer_id, curr;
  Peer* best_peer=NULL;
  Edge* edge=NULL, *other_edge=NULL;
  Channel* channel=NULL;
  uint64_t capacity, amt_to_send, fee, tmp_dist, weight, new_amt_to_receive;
  Array* hops=NULL;
  Path_hop* hop=NULL;


  while(heap_len(distance_heap[p])!=0)
    heap_pop(distance_heap[p], compare_distance);

  for(i=0; i<peer_index; i++){
    distance[p][i].peer = i;
    distance[p][i].distance = INF;
    distance[p][i].fee = 0;
    distance[p][i].amt_to_receive = 0;
    next_peer[p][i].edge = -1;
    next_peer[p][i].peer = -1;
  }

  distance[p][target].peer = target;
  distance[p][target].amt_to_receive = amount;
  distance[p][target].fee = 0;
  distance[p][target].distance = 0;


  distance_heap[p] =  heap_insert(distance_heap[p], &distance[p][target], compare_distance);


  while(heap_len(distance_heap[p])!=0) {
    d = heap_pop(distance_heap[p], compare_distance);
    best_peer_id = d->peer;
    if(best_peer_id==source) break;

    best_peer = array_get(peers, best_peer_id);

    for(j=0; j<array_len(best_peer->edge); j++) {
      // need to get other direction of the edge as search is performed from target to source
      other_edge_id = array_get(best_peer->edge, j);
      if(other_edge_id==NULL) continue;
      other_edge = array_get(edges, *other_edge_id);
      edge = array_get(edges, other_edge->other_edge_direction_id);

      prev_peer_id = other_edge->counterparty;
      edge_id = edge->ID;

      if(is_ignored(prev_peer_id, ignored_peers)) continue;
      if(is_ignored(edge_id, ignored_edges)) continue;

      to_node_dist = distance[p][best_peer_id];
      amt_to_send = to_node_dist.amt_to_receive;

      if(prev_peer_id==source)
        capacity = edge->balance;
      else{
        channel = array_get(channels, edge->channel_id);
        capacity = channel->capacity;
      }

      if(amt_to_send > capacity || amt_to_send < edge->policy.min_hTLC) continue;

      if(prev_peer_id==source)
        fee = 0;
      else
        fee = compute_fee(amt_to_send, edge->policy);

      new_amt_to_receive = amt_to_send + fee;

      weight = fee + new_amt_to_receive*edge->policy.timelock*15/1000000000;

      tmp_dist = weight + to_node_dist.distance;

      if(tmp_dist < distance[p][prev_peer_id].distance) {
        distance[p][prev_peer_id].peer = prev_peer_id;
        distance[p][prev_peer_id].distance = tmp_dist;
        distance[p][prev_peer_id].amt_to_receive = new_amt_to_receive;
        distance[p][prev_peer_id].fee = fee;

        next_peer[p][prev_peer_id].edge = edge_id;
        next_peer[p][prev_peer_id].peer = best_peer_id;

        distance_heap[p] = heap_insert(distance_heap[p], &distance[p][prev_peer_id], compare_distance);
      }
      }

    }


  if(next_peer[p][source].peer == -1) {
    return NULL;
  }


  hops = array_initialize(HOPSLIMIT);
  curr = source;
  while(curr!=target) {
    hop = malloc(sizeof(Path_hop));
    hop->sender = curr;
    hop->edge = next_peer[p][curr].edge;
    hop->receiver = next_peer[p][curr].peer;
    hops=array_insert(hops, hop);

    curr = next_peer[p][curr].peer;
  }


  if(array_len(hops)>HOPSLIMIT) {
    return NULL;
  }

  return hops;
}



Array* dijkstra(long source, long target, uint64_t amount, Array* ignored_peers, Array* ignored_edges) {
  Distance *d=NULL, to_node_dist;
  long i, best_peer_id, j,edge_id, *other_edge_id=NULL, prev_peer_id, curr;
  Peer* best_peer=NULL;
  Edge* edge=NULL, *other_edge=NULL;
  Channel* channel=NULL;
  uint64_t capacity, amt_to_send, fee, tmp_dist, weight, new_amt_to_receive;
  Array* hops=NULL;
  Path_hop* hop=NULL;

  printf("DIJKSTRA\n");

  while(heap_len(distance_heap[0])!=0)
    heap_pop(distance_heap[0], compare_distance);

  for(i=0; i<peer_index; i++){
    distance[0][i].peer = i;
    distance[0][i].distance = INF;
    distance[0][i].fee = 0;
    distance[0][i].amt_to_receive = 0;
    next_peer[0][i].edge = -1;
    next_peer[0][i].peer = -1;
  }

  distance[0][target].peer = target;
  distance[0][target].amt_to_receive = amount;
  distance[0][target].fee = 0;
  distance[0][target].distance = 0;


  distance_heap[0] =  heap_insert(distance_heap[0], &distance[0][target], compare_distance);


  while(heap_len(distance_heap[0])!=0) {
    d = heap_pop(distance_heap[0], compare_distance);
    best_peer_id = d->peer;
    if(best_peer_id==source) break;

    best_peer = array_get(peers, best_peer_id);

    for(j=0; j<array_len(best_peer->edge); j++) {
      // need to get other direction of the edge as search is performed from target to source
      other_edge_id = array_get(best_peer->edge, j);
      if(other_edge_id==NULL) continue;
      other_edge = array_get(edges, *other_edge_id);
      edge = array_get(edges, other_edge->other_edge_direction_id);

      prev_peer_id = other_edge->counterparty;
      edge_id = edge->ID;

      if(is_ignored(prev_peer_id, ignored_peers)) continue;
      if(is_ignored(edge_id, ignored_edges)) continue;

      to_node_dist = distance[0][best_peer_id];
      amt_to_send = to_node_dist.amt_to_receive;

      if(prev_peer_id==source)
        capacity = edge->balance;
      else{
        channel = array_get(channels, edge->channel_id);
        capacity = channel->capacity;
      }

      if(amt_to_send > capacity || amt_to_send < edge->policy.min_hTLC) continue;

      if(prev_peer_id==source)
        fee = 0;
      else
        fee = compute_fee(amt_to_send, edge->policy);

      new_amt_to_receive = amt_to_send + fee;

      weight = fee + new_amt_to_receive*edge->policy.timelock*15/1000000000;

      tmp_dist = weight + to_node_dist.distance;


      if(tmp_dist < distance[0][prev_peer_id].distance) {
        distance[0][prev_peer_id].peer = prev_peer_id;
        distance[0][prev_peer_id].distance = tmp_dist;
        distance[0][prev_peer_id].amt_to_receive = new_amt_to_receive;
        distance[0][prev_peer_id].fee = fee;

        next_peer[0][prev_peer_id].edge = edge_id;
        next_peer[0][prev_peer_id].peer = best_peer_id;

        distance_heap[0] = heap_insert(distance_heap[0], &distance[0][prev_peer_id], compare_distance);
      }
      }

    }


  if(next_peer[0][source].peer == -1) {
    return NULL;
  }


  hops = array_initialize(HOPSLIMIT);
  curr = source;
  while(curr!=target) {
    hop = malloc(sizeof(Path_hop));
    hop->sender = curr;
    hop->edge = next_peer[0][curr].edge;
    hop->receiver = next_peer[0][curr].peer;
    hops=array_insert(hops, hop);

    curr = next_peer[0][curr].peer;
  }


  if(array_len(hops)>HOPSLIMIT) {
    return NULL;
  }


  return hops;
}


int is_same_path(Array*root_path, Array*path) {
  long i;
  Path_hop* hop1, *hop2;

  for(i=0; i<array_len(root_path); i++) {
    hop1=array_get(root_path, i);
    hop2=array_get(path, i);
    if(hop1->edge != hop2->edge)
      return 0;
  }

  return 1;
}

int compare_path(Array* path1, Array* path2){
  long len1, len2;
  len1 = array_len(path1);
  len2=array_len(path2);

  if(len1==len2) return 0;
  else if (len1<len2) return -1;
  else return 1;
}



Route* route_initialize(long n_hops) {
  Route* r;

  r = malloc(sizeof(Route));
  r->route_hops = array_initialize(n_hops);
  r->total_amount = 0.0;
  r->total_fee = 0.0;
  r->total_timelock = 0;

  return r;
}


Route* transform_path_into_route(Array* path_hops, uint64_t amount_to_send, int final_timelock) {
  Path_hop *path_hop;
  Route_hop *route_hop, *next_route_hop;
  Route *route;
  long n_hops, i;
  uint64_t fee, current_channel_capacity;
  Edge* edge;
  Policy current_edge_policy, next_edge_policy;
  Channel* channel;

  n_hops = array_len(path_hops);
  route = route_initialize(n_hops);

  for(i=n_hops-1; i>=0; i--) {
    path_hop = array_get(path_hops, i);

    edge = array_get(edges, path_hop->edge);
    current_edge_policy = edge->policy;
    channel = array_get(channels,edge->channel_id);
    current_channel_capacity = channel->capacity;

    route_hop = malloc(sizeof(Route_hop));
    route_hop->path_hop = path_hop;

    if(i == array_len(path_hops)-1) {
      route_hop->amount_to_forward = amount_to_send;
      route->total_amount += amount_to_send;

      if(n_hops == 1) {
        route_hop->timelock = 0;
        route->total_timelock = 0;
      }
      else {
        route_hop->timelock = current_edge_policy.timelock;
        route->total_timelock += current_edge_policy.timelock;
      }
    }
    else {
      fee = compute_fee(next_route_hop->amount_to_forward, next_edge_policy);
      route_hop->amount_to_forward = next_route_hop->amount_to_forward + fee;
      route->total_fee += fee;
      route->total_amount += fee;

      route_hop->timelock = next_route_hop->timelock + current_edge_policy.timelock;
      route->total_timelock += current_edge_policy.timelock;
    }

    if(route_hop->amount_to_forward > current_channel_capacity)
      return NULL;

    route->route_hops = array_insert(route->route_hops, route_hop);
    next_edge_policy = current_edge_policy;
    next_route_hop = route_hop;

    }

  array_reverse(route->route_hops);

  return route;
  }


void print_hop(Route_hop* hop){
  printf("Sender %ld, Receiver %ld, Edge %ld\n", hop->path_hop->sender, hop->path_hop->receiver, hop->path_hop->edge);
}

/* version for not all peers (doesn't work)
void floyd_warshall() {
  long i, j, k, p;
  Channel* channel_info;
  Edge* direction1, *direction2;
  Path_hop *hop1, *hop2;
  uint16_t* d, *newd, dij, dik, dkj;
  Payment* payment;


  distht = hash_table_initialize(peer_index);
  nextht = hash_table_initialize(peer_index);

  for(i=0; i<channel_info_index; i++) {
    channel_info = hash_table_get(channel_infos, i);
    direction1 = hash_table_get(channels, channel_info->channel_direction1);
    direction2 = hash_table_get(channels, channel_info->channel_direction2);

    hash_table_matrix_put(distht, channel_info->peer1, channel_info->peer2, &(direction1->policy.timelock));
    hash_table_matrix_put(distht, channel_info->peer2, channel_info->peer1, &(direction2->policy.timelock));

    hop1 = malloc(sizeof(Path_hop));
    hop1->sender = channel_info->peer1;
    hop1->receiver = channel_info->peer2;
    hop1->channel = channel_info->channel_direction1;
    hash_table_matrix_put(nextht, channel_info->peer1, channel_info->peer2, hop1);

    hop2 = malloc(sizeof(Path_hop));
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


Array* get_path(long source, long destination) {
  Array* path;
  long next_peer;
  Path_hop* hop;

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
  Channel* channel_info;
  Edge* direction1, *direction2;

  dist = malloc(sizeof(uint32_t*)*peer_index);
  next = malloc(sizeof(Path_hop*)*peer_index);
  //  paths = malloc(sizeof(Array**)*peer_index);
  for(i=0; i<peer_index; i++) {
    dist[i] = malloc(sizeof(uint32_t)*peer_index);
    next[i] = malloc(sizeof(Path_hop)*peer_index);
    //paths[i] = malloc(sizeof(Array*)*peer_index);
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

Array* get_path(long source, long destination) {
  Array* path;
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

/* Array* dijkstra_p(long source, long target, uint64_t amount, Array* ignored_peers, Array* ignored_channels, long p) { */
/*   Distance *d=NULL; */
/*   long i, best_peer_id, j,*channel_id=NULL, next_peer_id, prev; */
/*   Peer* best_peer=NULL; */
/*   Edge* channel=NULL; */
/*   Channel* channel_info=NULL; */
/*   uint32_t tmp_dist; */
/*   uint64_t capacity; */
/*   Path_hop* hop=NULL; */
/*   Array* hops=NULL; */

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

/*       if(tmp_dist < distance[p][next_peer_id].distance && amount<=capacity && amount >= channel->policy.min_hTLC) { */
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

/*     hop = malloc(sizeof(Path_hop)); */

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


/* Array* dijkstra(long source, long target, uint64_t amount, Array* ignored_peers, Array* ignored_channels) { */
/*   Distance *d=NULL; */
/*   long i, best_peer_id, j,*channel_id=NULL, next_peer_id, prev; */
/*   Peer* best_peer=NULL; */
/*   Edge* channel=NULL; */
/*   Channel* channel_info=NULL; */
/*   uint32_t tmp_dist; */
/*   uint64_t capacity; */
/*   Array* hops=NULL; */
/*   Path_hop* hop=NULL; */

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
/*     hop = malloc(sizeof(Path_hop)); */
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



/* Array* find_paths(long source, long target, double amount){ */
/*   Array* starting_path, *first_path, *prev_shortest, *root_path, *path, *spur_path, *new_path, *next_shortest_path; */
/*   Array* ignored_channels, *ignored_peers; */
/*  Path_hop *hop; */
/*   long i, k, j, spur_node, new_path_len; */
/*   Array* shortest_paths; */
/*   Heap* candidate_paths; */

/*   candidate_paths = heap_initialize(100); */

/*   ignored_peers=array_initialize(2); */
/*   ignored_channels=array_initialize(2); */

/*   shortest_paths = array_initialize(100); */

/*   starting_path=dijkstra(source, target, amount, ignored_peers, ignored_channels); */
/*   if(starting_path==NULL) return NULL; */

/*   first_path = array_initialize(array_len(starting_path)+1); */
/*   hop = malloc(sizeof(Path_hop)); */
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
