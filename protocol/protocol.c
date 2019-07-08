#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gsl_rng.h"
#include "gsl_randist.h"
#include "gsl/gsl_math.h"
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include "protocol.h"
#include "../utils/array.h"
//#include "../utils/hash_table.h"
#include "../utils/heap.h"
#include "../simulator/initialize.h"
#include "routing.h"
#include "../simulator/event.h"
//#include "../simulator/stats.h"
#include "../global.h"

#define MAXMSATOSHI 5E17 //5 millions  bitcoin
#define MAXTIMELOCK 100
#define MINTIMELOCK 10
#define MAXFEEBASE 5000
#define MINFEEBASE 1000
#define MAXFEEPROP 10
#define MINFEEPROP 1
#define MAXLATENCY 100
#define MINLATENCY 10
#define MINBALANCE 1E2
#define MAXBALANCE 1E11
#define FAULTYLATENCY 3000 //3 seconds waiting for a peer not responding (tcp default retransmission time)
#define INF UINT16_MAX
#define HOPSLIMIT 20


long channel_index, peer_index, channel_info_index, payment_index;
gsl_rng *r;
const gsl_rng_type * T;
gsl_ran_discrete_t* uncoop_before_discrete, *uncoop_after_discrete;

Array* peers;
Array* channels;
Array* channel_infos;

long n_dijkstra;

Peer* create_peer(long ID, long channels_size) {
  Peer* peer;

  peer = malloc(sizeof(Peer));
  peer->ID=ID;
  peer->channel = array_initialize(channels_size);
  peer->initial_funds = 0;
  peer->remaining_funds = 0;
  peer->withholds_r = 0;

  peer_index++;

  return peer;
}

Channel_info* create_channel_info(long ID, long peer1, long peer2, uint32_t latency) {
  Channel_info* channel_info;

  channel_info = malloc(sizeof(Channel_info));
  channel_info->ID = ID;
  channel_info->peer1 = peer1;
  channel_info->peer2 = peer2;
  channel_info->latency = latency;
  channel_info->capacity = 0;
  channel_info->is_closed = 0;

  channel_info_index++;

  return channel_info;
}


Channel* create_channel(long ID, long channel_info_iD, long counterparty, Policy policy){
  Channel* channel;

  channel = malloc(sizeof(Channel));
  channel->ID = ID;
  channel->channel_info_iD = channel_info_iD;
  channel->counterparty = counterparty;
  channel->policy = policy;
  channel->balance = 0;
  channel->is_closed = 0;

  channel_index++;

  return channel;
}

Payment* create_payment(long ID, long sender, long receiver, uint64_t amount) {
  Payment * p;

  p = malloc(sizeof(Payment));
  p->ID=ID;
  p->sender= sender;
  p->receiver = receiver;
  p->amount = amount;
  p->route = NULL;
  p->is_success = 0;
  p->uncoop_after = 0;
  p->uncoop_before=0;
  p->is_timeout = 0;
  p->start_time = 0;
  p->end_time = 0;
  p->attempts = 0;

  payment_index++;

  return p;
}

Peer* create_peer_post_proc(long ID, int withholds_r) {
  Peer* peer;

  peer = malloc(sizeof(Peer));
  peer->ID=ID;
  peer->channel = array_initialize(5);
  peer->initial_funds = 0;
  peer->remaining_funds = 0;
  peer->withholds_r = withholds_r;
  peer->ignored_channels = array_initialize(10);
  peer->ignored_peers = array_initialize(1);

  peer_index++;

  return peer;
}

Channel_info* create_channel_info_post_proc(long ID, long direction1, long direction2, long peer1, long peer2, uint64_t capacity, uint32_t latency) {
  Channel_info* channel_info;

  channel_info = malloc(sizeof(Channel_info));
  channel_info->ID = ID;
  channel_info->channel_direction1 = direction1;
  channel_info->channel_direction2 = direction2;
  channel_info->peer1 = peer1;
  channel_info->peer2 = peer2;
  channel_info->latency = latency;
  channel_info->capacity = capacity;
  channel_info->is_closed = 0;

  channel_info_index++;

  return channel_info;
}

Channel* create_channel_post_proc(long ID, long channel_info_iD, long other_direction, long counterparty, uint64_t balance, Policy policy){
  Channel* channel;

  channel = malloc(sizeof(Channel));
  channel->ID = ID;
  channel->channel_info_iD = channel_info_iD;
  channel->counterparty = counterparty;
  channel->other_channel_direction_iD = other_direction;
  channel->policy = policy;
  channel->balance = balance;
  channel->is_closed = 0;

  channel_index++;

  return channel;
}





void compute_peers_initial_funds(double gini) {
  long i;
  Peer* peer;

  for(i=0; i<peer_index; i++) {
    peer = array_get(peers, i);
    peer->initial_funds = MAXMSATOSHI/peer_index;
    peer->remaining_funds = peer->initial_funds;
  }
}


double compute_gini() {
  long i, j;
  __uint128_t num=0, den=0;
  __uint128_t difference;
  Channel_info *channeli, *channelj;
  double gini;

  for(i=0;i<channel_info_index; i++) {
    channeli = array_get(channel_infos, i);
    den += channeli->capacity;
    for(j=0; j<channel_info_index; j++){
      if(i==j) continue;
      channelj = array_get(channel_infos, j);
      if(channeli->capacity > channelj->capacity)
        difference = channeli->capacity - channelj->capacity;
      else
        difference = channelj->capacity - channeli->capacity;
      num += difference;
    }
  }

  den = 2*channel_info_index*den;

  gini = num/(den*1.0);

  return gini;
}

void initialize_topology_preproc(long n_peers, long n_channels, double RWithholding, int gini, int sigma, long capacity_per_channel) {
  long i, j, peer_iDIndex, channel_info_iDIndex, channel_iDIndex, peer1, peer2, direction1, direction2, info;
  double Rwithholding_p[] = {1-RWithholding, RWithholding}, balance_p[] = {0.5, 0.5}, gini_p[3], min_hTLCP[]={0.7, 0.2, 0.05, 0.05},coeff1, coeff2, mean=n_peers/2 ;
  gsl_ran_discrete_t* Rwithholding_discrete, *balance_discrete, *gini_discrete, *min_hTLCDiscrete;
  int *peer_channels;
  uint32_t latency;
  uint64_t balance1, balance2, capacity, min_hTLC;
  Policy policy1, policy2;
  long thresh1, thresh2, counter=0;
  uint64_t funds[3], maxfunds, funds_part;

  switch(gini) {
  case 1:
    coeff1 = 1.0/3;
    coeff2 = 1.0/3;
    break;
  case 2:
    coeff1 = 1.0/6;
    coeff2 = 1.0/3;
    break;
  case 3:
    coeff1 = 1.0/8;
    coeff2 = 1.0/6;
    break;
  case 4:
    coeff1 = 1.0/16;
    coeff2 = 1.0/12;
    break;
  case 5:
    coeff1 = 1.0/1000;
    coeff2 = 1.0/1000;
    break;
  default:
    printf("ERROR wrong preinput gini level\n");
    return;
  }

  gini_p[0] = coeff1;
  gini_p[1] = coeff2;
  gini_p[2] = 1 - (coeff1+coeff2);

  gini_discrete = gsl_ran_discrete_preproc(3, gini_p);

  thresh1 = n_peers*n_channels*coeff1;
  thresh2 = n_peers*n_channels*coeff2;

  // NOTE: *5 instead of *n_channels if you want to test Gini
  funds_part = (capacity_per_channel/3)*n_peers*n_channels;

//  printf("%ld, %ld\n", thresh1, thresh2);

  /* if(gini != 5) { */
  funds[0] = (funds_part)/thresh1;
  funds[1] = (funds_part)/(thresh2);
  funds[2] = (funds_part)/ (n_peers*n_channels - (thresh1 + thresh2));

  csv_peer = fopen("peer.csv", "w");
  if(csv_peer==NULL) {
    printf("ERROR cannot open file peer.csv\n");
    return;
  }
  fprintf(csv_peer, "id,withholds_r\n");

  csv_channel_info = fopen("channel_info.csv", "w");
  if(csv_channel_info==NULL) {
    printf("ERROR cannot open file channel_info.csv\n");
    return;
  }
  fprintf(csv_channel_info, "id,direction1,direction2,peer1,peer2,capacity,latency\n");

  csv_channel = fopen("channel.csv", "w");
  if(csv_channel==NULL) {
    printf("ERROR cannot open file channel.csv\n");
    return;
  }
  fprintf(csv_channel, "id,channel_info,other_direction,counterparty,balance,fee_base,fee_proportional,min_htlc,timelock\n");


  Rwithholding_discrete = gsl_ran_discrete_preproc(2, Rwithholding_p);
  balance_discrete = gsl_ran_discrete_preproc(2, balance_p);


  peer_iDIndex=0;
  for(i=0; i<n_peers; i++){
    fprintf(csv_peer, "%ld,%ld\n", peer_iDIndex, gsl_ran_discrete(r, Rwithholding_discrete));
    peer_iDIndex++;
  }

  peer_channels = malloc(peer_iDIndex*sizeof(int));
  for(i=0;i<peer_iDIndex;i++) 
    peer_channels[i] = 0;

  channel_info_iDIndex = channel_iDIndex= 0;
  for(i=0; i<peer_iDIndex; i++) {
    peer1 = i;
    for(j=0; j<n_channels &&  (peer_channels[peer1] < n_channels); j++){

      do {
       if(j==0 && sigma!=-1) {
          peer2 = mean + gsl_ran_gaussian(r, sigma);
          }
          else
            peer2 = gsl_rng_uniform_int(r,peer_iDIndex);
      }while(peer2==peer1);

      if(peer2<0 || peer2>=peer_iDIndex) continue;

      if(sigma!=-1) {
        if(j!=0 && peer_channels[peer2]>=n_channels) continue;
      }
      else {
        if(peer_channels[peer2]>=n_channels) continue;
      }



      ++peer_channels[peer1];
      ++peer_channels[peer2];
      info = channel_info_iDIndex;
      ++channel_info_iDIndex;
      direction1 = channel_iDIndex;
      ++channel_iDIndex;
      direction2 = channel_iDIndex;
      ++channel_iDIndex;

      latency = gsl_rng_uniform_int(r, MAXLATENCY - MINLATENCY) + MINLATENCY;

      capacity = funds[gsl_ran_discrete(r, gini_discrete)];


      double balance_mean=5, balance_sigma=2.0, fraction;
      int gauss;

      gauss=gsl_ran_gaussian(r, balance_sigma);

      if(gsl_ran_discrete(r, balance_discrete))
        gauss = 2+gauss;
      else
        gauss = 7+gauss;

      if(gauss>10) gauss=10;
      if(gauss<0) gauss = 0;

      fraction = gauss/10.0;
      balance1 = fraction*capacity;
      balance2 = capacity - balance1;


      min_hTLCDiscrete = gsl_ran_discrete_preproc(4, min_hTLCP);


      policy1.fee_base = gsl_rng_uniform_int(r, MAXFEEBASE - MINFEEBASE) + MINFEEBASE;
      policy1.fee_proportional = (gsl_rng_uniform_int(r, MAXFEEPROP-MINFEEPROP)+MINFEEPROP);
      policy1.timelock = gsl_rng_uniform_int(r, MAXTIMELOCK-MINTIMELOCK)+MINTIMELOCK;
      policy1.min_hTLC = gsl_pow_int(10, gsl_ran_discrete(r, min_hTLCDiscrete));
      policy1.min_hTLC = policy1.min_hTLC == 1 ? 0 : policy1.min_hTLC;
      policy2.fee_base = gsl_rng_uniform_int(r, MAXFEEBASE - MINFEEBASE) + MINFEEBASE;
      policy2.fee_proportional = (gsl_rng_uniform_int(r, MAXFEEPROP-MINFEEPROP)+MINFEEPROP);
      policy2.timelock = gsl_rng_uniform_int(r, MAXTIMELOCK-MINTIMELOCK)+MINTIMELOCK;
      policy2.min_hTLC = gsl_pow_int(10, gsl_ran_discrete(r, min_hTLCDiscrete));
      policy2.min_hTLC = policy2.min_hTLC == 1 ? 0 : policy2.min_hTLC;

      fprintf(csv_channel_info, "%ld,%ld,%ld,%ld,%ld,%ld,%d\n", info, direction1, direction2, peer1, peer2, capacity, latency);

      fprintf(csv_channel, "%ld,%ld,%ld,%ld,%ld,%d,%d,%d,%d\n", direction1, info, direction2, peer2, balance1, policy1.fee_base, policy1.fee_proportional, policy1.min_hTLC, policy1.timelock);

      fprintf(csv_channel, "%ld,%ld,%ld,%ld,%ld,%d,%d,%d,%d\n", direction2, info, direction1, peer1, balance2, policy2.fee_base, policy2.fee_proportional, policy2.min_hTLC, policy2.timelock);

    }

  }


/*   double num = 0, den = 0; */
/*   for(i=0; i<peer_iDIndex; i++) { */
/*     //    if(peer_channels[i]>5) printf("%ld, %d\n", i, peer_channels[i]); */
/*     // else { */
/*     if(peer_channels[i]>5) */
/*       printf("%ld, %d\n", i, peer_channels[i]); */
/*     den++; */
/*     num += peer_channels[i]; */
/*       //} */
/*   } */

/*   printf("mean channels: %lf\n", num/den); */

/* exit(-1); */

  fclose(csv_peer);
  fclose(csv_channel_info);
  fclose(csv_channel);


}


void create_topology_from_csv(unsigned int is_preproc) {
  char row[256], channel_file[64], info_file[64], peer_file[64];
  Peer* peer, *peer1, *peer2;
  long ID, direction1, direction2, peer_iD1, peer_iD2, channel_info_iD, other_direction, counterparty;
  Policy policy;
  int withholds_r;
  uint32_t latency;
  uint64_t capacity, balance;
  Channel_info* channel_info;
  Channel* channel;

  if(is_preproc) {
    strcpy(channel_file, "channel.csv");
    strcpy(info_file, "channel_info.csv");
    strcpy(peer_file, "peer.csv");
  }
  else {
    strcpy(channel_file, "channel_ln.csv");
    strcpy(info_file, "channel_info_ln.csv");
    strcpy(peer_file, "peer_ln.csv");
    }


  csv_peer = fopen(peer_file, "r");
  if(csv_peer==NULL) {
    printf("ERROR cannot open file peer.csv\n");
    return;
  }

  fgets(row, 256, csv_peer);
  while(fgets(row, 256, csv_peer)!=NULL) {
    sscanf(row, "%ld,%d", &ID, &withholds_r);
    peer = create_peer_post_proc(ID, withholds_r);
    peers = array_insert(peers, peer);
  }

  fclose(csv_peer);

  csv_channel_info = fopen(info_file, "r");
  if(csv_channel_info==NULL) {
    printf("ERROR cannot open file %s\n", info_file);
    return;
  }

  fgets(row, 256, csv_channel_info);
  while(fgets(row, 256, csv_channel_info)!=NULL) {
    sscanf(row, "%ld,%ld,%ld,%ld,%ld,%ld,%d", &ID, &direction1, &direction2, &peer_iD1, &peer_iD2, &capacity, &latency);
    channel_info = create_channel_info_post_proc(ID, direction1, direction2, peer_iD1, peer_iD2, capacity, latency);
    channel_infos = array_insert(channel_infos, channel_info);
    peer1 = array_get(peers, peer_iD1);
    peer1->channel = array_insert(peer1->channel, &(channel_info->channel_direction1));
    peer2 = array_get(peers, peer_iD2);
    peer2->channel = array_insert(peer2->channel, &(channel_info->channel_direction2));
  }

  fclose(csv_channel_info);

  csv_channel = fopen(channel_file, "r");
  if(csv_channel==NULL) {
    printf("ERROR cannot open file %s\n", channel_file);
    return;
  }

  fgets(row, 256, csv_channel);
  while(fgets(row, 256, csv_channel)!=NULL) {
    sscanf(row, "%ld,%ld,%ld,%ld,%ld,%d,%d,%d,%hd", &ID, &channel_info_iD, &other_direction, &counterparty, &balance, &policy.fee_base, &policy.fee_proportional, &policy.min_hTLC, &policy.timelock);
    channel = create_channel_post_proc(ID, channel_info_iD, other_direction, counterparty, balance, policy);
    channels = array_insert(channels, channel);
  }

  fclose(csv_channel);

}



void initialize_protocol_data(long n_peers, long n_channels, double p_uncoop_before, double p_uncoop_after, double RWithholding, int gini, int sigma, long capacity_per_channel, unsigned int is_preproc) {
  double before_p[] = {p_uncoop_before, 1-p_uncoop_before};
  double after_p[] = {p_uncoop_after, 1-p_uncoop_after};

  gsl_rng_env_setup();
  T = gsl_rng_default;
  r = gsl_rng_alloc (T);

  uncoop_before_discrete = gsl_ran_discrete_preproc(2, before_p);
  uncoop_after_discrete = gsl_ran_discrete_preproc(2, after_p);

  channel_index = peer_index = channel_info_index = payment_index = 0;

  n_dijkstra = 0;

  peers = array_initialize(n_peers);
  channels = array_initialize(n_channels*n_peers);
  channel_infos = array_initialize(n_channels*n_peers);


  if(is_preproc)
    initialize_topology_preproc(n_peers, n_channels, RWithholding, gini, sigma, capacity_per_channel);

  create_topology_from_csv(is_preproc);



}


int is_present(long element, Array* long_array) {
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


void close_channel(long channel_info_iD) {
  long i;
  Peer *peer;
  Channel_info *channel_info;
  Channel* direction1, *direction2;

  channel_info = array_get(channel_infos, channel_info_iD);
  direction1 = array_get(channels, channel_info->channel_direction1);
  direction2 = array_get(channels, channel_info->channel_direction2);

  channel_info->is_closed = 1;
  direction1->is_closed = 1;
  direction2->is_closed = 1;

  printf("Channel_info %ld, Channel_direction1 %ld, Channel_direction2 %ld are now closed\n", channel_info->ID, channel_info->channel_direction1, channel_info->channel_direction2);

  for(i = 0; i < peer_index; i++) {
    peer = array_get(peers, i);
    array_delete(peer->channel, &(channel_info->channel_direction1), is_equal);
    array_delete(peer->channel, &(channel_info->channel_direction2), is_equal);
  }
}


int is_cooperative_before_lock() {
  return gsl_ran_discrete(r, uncoop_before_discrete);
}

int is_cooperative_after_lock() {
  return gsl_ran_discrete(r, uncoop_after_discrete);
}

uint64_t compute_fee(uint64_t amount_to_forward, Policy policy) {
  uint64_t fee;
  fee = (policy.fee_proportional*amount_to_forward) / 1000000;
  return policy.fee_base + fee;
}

void* dijkstra_thread(void*arg) {
  Payment * payment;
  Array* hops;
  long payment_iD;
  int *index;
  Peer* peer;

  index = (int*) arg;


  while(1) {
    pthread_mutex_lock(&jobs_mutex);
    jobs = pop(jobs, &payment_iD);
    pthread_mutex_unlock(&jobs_mutex);

    if(payment_iD==-1) return NULL;

    pthread_mutex_lock(&peers_mutex);
    payment = array_get(payments, payment_iD);
    peer = array_get(peers, payment->sender);
    pthread_mutex_unlock(&peers_mutex);

    printf("DIJKSTRA %ld\n", payment->ID);

    hops = dijkstra_p(payment->sender, payment->receiver, payment->amount, peer->ignored_peers,
                     peer->ignored_channels, *index);


    paths[payment->ID] = hops;
  }

  return NULL;

}

unsigned int is_any_channel_closed(Array* hops) {
  int i;
  Channel* channel;
  Path_hop* hop;

  for(i=0;i<array_len(hops);i++) {
    hop = array_get(hops, i);
    channel = array_get(channels, hop->channel);
    if(channel->is_closed)
      return 1;
  }

  return 0;
}

int is_equal_ignored(Ignored* a, Ignored* b){
  return a->ID == b->ID;
}

void check_ignored(long sender_iD){
  Peer* sender;
  Array* ignored_channels;
  Ignored* ignored;
  int i;

  sender = array_get(peers, sender_iD);
  ignored_channels = sender->ignored_channels;

  for(i=0; i<array_len(ignored_channels); i++){
    ignored = array_get(ignored_channels, i);

    //register time of newly added ignored channels
    if(ignored->time==0)
      ignored->time = simulator_time;

    //remove decayed ignored channels
    if(simulator_time > 5000 + ignored->time){
      array_delete(ignored_channels, ignored, is_equal_ignored);
    }
  }
}


void find_route(Event *event) {
  Payment *payment;
  Peer* peer;
  Array *path_hops;
  Route* route;
  int final_timelock=9;
  Event* send_event;
  uint64_t next_event_time;

  printf("FINDROUTE %ld\n", event->payment_iD);

  payment = array_get(payments, event->payment_iD);
  peer = array_get(peers, payment->sender);

  ++(payment->attempts);

  if(payment->start_time < 1)
    payment->start_time = simulator_time;

  /*
  // dijkstra version
  path_hops = dijkstra(payment->sender, payment->receiver, payment->amount, payment->ignored_peers,
                      payment->ignored_channels);
*/  

  /* floyd_warshall version
  if(payment->attempts == 0) {
    path_hops = get_path(payment->sender, payment->receiver);
  }
  else {
    path_hops = dijkstra(payment->sender, payment->receiver, payment->amount, payment->ignored_peers,
                        payment->ignored_channels);
                        }*/

  //dijkstra parallel OLD version
  /* if(payment->attempts > 0) */
  /*   pthread_create(&tid, NULL, &dijkstra_thread, payment); */

  /* pthread_mutex_lock(&(cond_mutex[payment->ID])); */
  /* while(!cond_paths[payment->ID]) */
  /*   pthread_cond_wait(&(cond_var[payment->ID]), &(cond_mutex[payment->ID])); */
  /* cond_paths[payment->ID] = 0; */
  /* pthread_mutex_unlock(&(cond_mutex[payment->ID])); */

  if(simulator_time > payment->start_time + 60000) {
    payment->end_time = simulator_time;
    payment->is_timeout = 1;
    return;
  }

  check_ignored(payment->sender);

  //dijkstra parallel NEW version
  if(payment->attempts==1) {
    path_hops = paths[payment->ID];
    if(path_hops!=NULL)
      if(is_any_channel_closed(path_hops)) {
        path_hops = dijkstra(payment->sender, payment->receiver, payment->amount, peer->ignored_peers,
                            peer->ignored_channels);
        n_dijkstra++;
      }
  }
  else {
    path_hops = dijkstra(payment->sender, payment->receiver, payment->amount, peer->ignored_peers,
                        peer->ignored_channels);
    n_dijkstra++;
  }

  if(path_hops!=NULL)
    if(is_any_channel_closed(path_hops)) {
    path_hops = dijkstra(payment->sender, payment->receiver, payment->amount, peer->ignored_peers,
                                       peer->ignored_channels);
    paths[payment->ID] = path_hops;
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
  send_event = create_event(event_index, next_event_time, SENDPAYMENT, payment->sender, event->payment_iD );
  events = heap_insert(events, send_event, compare_event);

}

Route_hop *get_route_hop(long peer_iD, Array *route_hops, int is_sender) {
  Route_hop *route_hop;
  long i, index = -1;

  for (i = 0; i < array_len(route_hops); i++) {
    route_hop = array_get(route_hops, i);

    if (is_sender && route_hop->path_hop->sender == peer_iD) {
      index = i;
      break;
    }

    if (!is_sender && route_hop->path_hop->receiver == peer_iD) {
      index = i;
      break;
    }
  }

  if (index == -1)
    return NULL;

  return array_get(route_hops, index);
}

int check_policy_forward( Route_hop* prev_hop, Route_hop* curr_hop) {
  Policy policy;
  Channel* curr_channel, *prev_channel;
  uint64_t fee;

  curr_channel = array_get(channels, curr_hop->path_hop->channel);
  prev_channel = array_get(channels, prev_hop->path_hop->channel);



  fee = compute_fee(curr_hop->amount_to_forward,curr_channel->policy);
  //the check should be: prev_hop->amount_to_forward - fee != curr_hop->amount_to_forward
  if(prev_hop->amount_to_forward - fee != curr_hop->amount_to_forward) {
    printf("ERROR: Fee not respected\n");
    printf("Prev_hop_amount %ld - fee %ld != Curr_hop_amount %ld\n", prev_hop->amount_to_forward, fee, curr_hop->amount_to_forward);
    print_hop(curr_hop);
    return 0;
  }

  if(prev_hop->timelock - prev_channel->policy.timelock != curr_hop->timelock) {
    printf("ERROR: Timelock not respected\n");
    printf("Prev_hop_timelock %d - policy_timelock %d != curr_hop_timelock %d \n",prev_hop->timelock, policy.timelock, curr_hop->timelock);
    print_hop(curr_hop);
    return 0;
  }

  return 1;
}


void add_ignored(long peer_iD, long ignored_iD){
  Ignored* ignored;
  Peer* peer;

  ignored = (Ignored*)malloc(sizeof(Ignored));
  ignored->ID = ignored_iD;
  ignored->time = 0;

  peer = array_get(peers, peer_iD);
  peer->ignored_channels = array_insert(peer->ignored_channels, ignored);
}


void send_payment(Event* event) {
  Payment* payment;
  uint64_t next_event_time;
  Route* route;
  Route_hop* first_route_hop;
  Channel* next_channel;
  Channel_info* channel_info;
  Event* next_event;
  Event_type event_type;
  Peer* peer;


  payment = array_get(payments, event->payment_iD);
  peer = array_get(peers, event->peer_iD);
  route = payment->route;
  first_route_hop = array_get(route->route_hops, 0);
  next_channel = array_get(channels, first_route_hop->path_hop->channel);
  channel_info = array_get(channel_infos, next_channel->channel_info_iD);

  if(!is_present(next_channel->ID, peer->channel)) {
    printf("Channel %ld has been closed\n", next_channel->ID);
    next_event_time = simulator_time;
    next_event = create_event(event_index, next_event_time, FINDROUTE, event->peer_iD, event->payment_iD );
    events = heap_insert(events, next_event, compare_event);
    return;
  }


  if(first_route_hop->amount_to_forward > next_channel->balance) {
    printf("Not enough balance in channel %ld\n", next_channel->ID);
    add_ignored(payment->sender, next_channel->ID);
    next_event_time = simulator_time;
    next_event = create_event(event_index, next_event_time, FINDROUTE, event->peer_iD, event->payment_iD );
    events = heap_insert(events, next_event, compare_event);
    return;
  }

  next_channel->balance -= first_route_hop->amount_to_forward;


  event_type = first_route_hop->path_hop->receiver == payment->receiver ? RECEIVEPAYMENT : FORWARDPAYMENT;
  next_event_time = simulator_time + channel_info->latency;
  next_event = create_event(event_index, next_event_time, event_type, first_route_hop->path_hop->receiver, event->payment_iD );
  events = heap_insert(events, next_event, compare_event);
}

void forward_payment(Event *event) {
  Payment* payment;
  Route* route;
  Route_hop* next_route_hop, *previous_route_hop;
  long  next_peer_iD, prev_peer_iD;
  Event_type event_type;
  Event* next_event;
  uint64_t next_event_time;
  Channel_info *prev_channel_info, *next_channel_info;
  Channel* prev_channel, *next_channel;
  int is_policy_respected;
  Peer* peer;

  payment = array_get(payments, event->payment_iD);
  peer = array_get(peers, event->peer_iD);
  route = payment->route;
  next_route_hop=get_route_hop(peer->ID, route->route_hops, 1);
  previous_route_hop = get_route_hop(peer->ID, route->route_hops, 0);
  prev_channel = array_get(channels, previous_route_hop->path_hop->channel);
  next_channel = array_get(channels, next_route_hop->path_hop->channel);
  prev_channel_info = array_get(channel_infos, prev_channel->channel_info_iD);
  next_channel_info = array_get(channel_infos, next_channel->channel_info_iD);

  if(next_route_hop == NULL || previous_route_hop == NULL) {
    printf("ERROR: no route hop\n");
    return;
  }

  if(!is_present(next_route_hop->path_hop->channel, peer->channel)) {
    printf("Channel %ld has been closed\n", next_route_hop->path_hop->channel);
    prev_peer_iD = previous_route_hop->path_hop->sender;
    event_type = prev_peer_iD == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    next_event_time = simulator_time + prev_channel_info->latency;
    next_event = create_event(event_index, next_event_time, event_type, prev_peer_iD, event->payment_iD);
    events = heap_insert(events, next_event, compare_event);
    return;
  }

  
  if(!is_cooperative_before_lock()){
    printf("Peer %ld is not cooperative before lock\n", event->peer_iD);
    payment->uncoop_before = 1;
    add_ignored(payment->sender, prev_channel->ID);

    prev_peer_iD = previous_route_hop->path_hop->sender;
    event_type = prev_peer_iD == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    next_event_time = simulator_time + prev_channel_info->latency + FAULTYLATENCY;
    next_event = create_event(event_index, next_event_time, event_type, prev_peer_iD, event->payment_iD );
    events = heap_insert(events, next_event, compare_event);
    return;
  }


  if(!is_cooperative_after_lock()) {
    printf("Peer %ld is not cooperative after lock on channel %ld\n", event->peer_iD, prev_channel->ID);
    payment->uncoop_after = 1;
    close_channel(prev_channel->channel_info_iD);

    payment->is_success = 0;
    payment->end_time = simulator_time;
    return;
  }



  is_policy_respected = check_policy_forward(previous_route_hop, next_route_hop);
  if(!is_policy_respected) return;

  if(next_route_hop->amount_to_forward > next_channel->balance ) {
    printf("Not enough balance in channel %ld\n", next_channel->ID);
    add_ignored(payment->sender, next_channel->ID);

    prev_peer_iD = previous_route_hop->path_hop->sender;
    event_type = prev_peer_iD == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    next_event_time = simulator_time + prev_channel_info->latency;
    next_event = create_event(event_index, next_event_time, event_type, prev_peer_iD, event->payment_iD );
    events = heap_insert(events, next_event, compare_event);
    return;
  }
  next_channel->balance -= next_route_hop->amount_to_forward;


  next_peer_iD = next_route_hop->path_hop->receiver;
  event_type = next_peer_iD == payment->receiver ? RECEIVEPAYMENT : FORWARDPAYMENT;
  next_event_time = simulator_time + next_channel_info->latency;
  next_event = create_event(event_index, next_event_time, event_type, next_peer_iD, event->payment_iD );
  events = heap_insert(events, next_event, compare_event);

}

void receive_payment(Event* event ) {
  long peer_iD, prev_peer_iD;
  Route* route;
  Payment* payment;
  Route_hop* last_route_hop;
  Channel* forward_channel,*backward_channel;
  Channel_info* channel_info;
  Event* next_event;
  Event_type event_type;
  uint64_t next_event_time;
  Peer* peer;

  peer_iD = event->peer_iD;
  peer = array_get(peers, peer_iD);
  payment = array_get(payments, event->payment_iD);
  route = payment->route;

  last_route_hop = array_get(route->route_hops, array_len(route->route_hops) - 1);
  forward_channel = array_get(channels, last_route_hop->path_hop->channel);
  backward_channel = array_get(channels, forward_channel->other_channel_direction_iD);
  channel_info = array_get(channel_infos, forward_channel->channel_info_iD);

  backward_channel->balance += last_route_hop->amount_to_forward;

  if(!is_cooperative_before_lock()){
    printf("Peer %ld is not cooperative before lock\n", event->peer_iD);
    payment->uncoop_before = 1;
    add_ignored(payment->sender, forward_channel->ID);

    prev_peer_iD = last_route_hop->path_hop->sender;
    event_type = prev_peer_iD == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    next_event_time = simulator_time + channel_info->latency + FAULTYLATENCY;
    next_event = create_event(event_index, next_event_time, event_type, prev_peer_iD, event->payment_iD );
    events = heap_insert(events, next_event, compare_event);
    return;
  }

  payment->is_success = 1;

  prev_peer_iD = last_route_hop->path_hop->sender;
  event_type = prev_peer_iD == payment->sender ? RECEIVESUCCESS : FORWARDSUCCESS;
  next_event_time = simulator_time + channel_info->latency;
  next_event = create_event(event_index, next_event_time, event_type, prev_peer_iD, event->payment_iD);
  events = heap_insert(events, next_event, compare_event);
}


void forward_success(Event* event) {
  Route_hop* prev_hop, *next_hop;
  Payment* payment;
  Channel* forward_channel, * backward_channel, *next_channel;
  Channel_info *prev_channel_info;
  long prev_peer_iD;
  Event* next_event;
  Event_type event_type;
  Peer* peer;
  uint64_t next_event_time;


  payment = array_get(payments, event->payment_iD);
  prev_hop = get_route_hop(event->peer_iD, payment->route->route_hops, 0);
  next_hop = get_route_hop(event->peer_iD, payment->route->route_hops, 1);
  next_channel = array_get(channels, next_hop->path_hop->channel);
  forward_channel = array_get(channels, prev_hop->path_hop->channel);
  backward_channel = array_get(channels, forward_channel->other_channel_direction_iD);
  prev_channel_info = array_get(channel_infos, forward_channel->channel_info_iD);
  peer = array_get(peers, event->peer_iD);
 

  if(!is_present(backward_channel->ID, peer->channel)) {
    printf("Channel %ld is not present\n", prev_hop->path_hop->channel);
    prev_peer_iD = prev_hop->path_hop->sender;
    event_type = prev_peer_iD == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
    next_event_time = simulator_time + prev_channel_info->latency;
    next_event = create_event(event_index, next_event_time, event_type, prev_peer_iD, event->payment_iD);
    events = heap_insert(events, next_event, compare_event);
    return;
  }



  if(!is_cooperative_after_lock()) {
    printf("Peer %ld is not cooperative after lock on channel %ld\n", event->peer_iD, next_channel->ID);
    payment->uncoop_after = 1;
    close_channel(next_channel->channel_info_iD);

    payment->is_success = 0;
    payment->end_time = simulator_time;

    return;
  }



  backward_channel->balance += prev_hop->amount_to_forward;


  prev_peer_iD = prev_hop->path_hop->sender;
  event_type = prev_peer_iD == payment->sender ? RECEIVESUCCESS : FORWARDSUCCESS;
  next_event_time = simulator_time + prev_channel_info->latency;
  next_event = create_event(event_index, next_event_time, event_type, prev_peer_iD, event->payment_iD);
  events = heap_insert(events, next_event, compare_event);
}

void receive_success(Event* event){
  Payment *payment;
  printf("RECEIVE SUCCESS %ld\n", event->payment_iD);
  payment = array_get(payments, event->payment_iD);
  payment->end_time = simulator_time;
}


void forward_fail(Event* event) {
  Payment* payment;
  Route_hop* next_hop, *prev_hop;
  Channel* next_channel, *prev_channel;
  Channel_info *prev_channel_info;
  long prev_peer_iD;
  Event* next_event;
  Event_type event_type;
  Peer* peer;
  uint64_t next_event_time;

 
  peer = array_get(peers, event->peer_iD); 
  payment = array_get(payments, event->payment_iD);
  next_hop = get_route_hop(event->peer_iD, payment->route->route_hops, 1);
  next_channel = array_get(channels, next_hop->path_hop->channel);

  if(is_present(next_channel->ID, peer->channel)) {
    next_channel->balance += next_hop->amount_to_forward;
  }
  else
    printf("Channel %ld is not present\n", next_hop->path_hop->channel);

  prev_hop = get_route_hop(event->peer_iD, payment->route->route_hops, 0);
  prev_channel = array_get(channels, prev_hop->path_hop->channel);
  prev_channel_info = array_get(channel_infos, prev_channel->channel_info_iD);
  prev_peer_iD = prev_hop->path_hop->sender;
  event_type = prev_peer_iD == payment->sender ? RECEIVEFAIL : FORWARDFAIL;
  next_event_time = simulator_time + prev_channel_info->latency;
  next_event = create_event(event_index, next_event_time, event_type, prev_peer_iD, event->payment_iD);
  events = heap_insert(events, next_event, compare_event);
}


void receive_fail(Event* event) {
  Payment* payment;
  Route_hop* first_hop;
  Channel* next_channel;
  Event* next_event;
  Peer* peer;
  uint64_t next_event_time;

  printf("RECEIVE FAIL %ld\n", event->payment_iD);

  payment = array_get(payments, event->payment_iD);
  first_hop = array_get(payment->route->route_hops, 0);
  next_channel = array_get(channels, first_hop->path_hop->channel);
  peer = array_get(peers, event->peer_iD);

  if(is_present(next_channel->ID, peer->channel))
    next_channel->balance += first_hop->amount_to_forward;
  else
    printf("Channel %ld is not present\n", next_channel->ID);

  if(payment->is_success == 1 ) {
    payment->end_time = simulator_time;
    return; //it means that money actually arrived to the destination but a peer was not cooperative when forwarding the success
  }

  next_event_time = simulator_time;
  next_event = create_event(event_index, next_event_time, FINDROUTE, payment->sender, payment->ID);
  events = heap_insert(events, next_event, compare_event);
}

