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


long edge_index, node_index, channel_index, payment_index;
gsl_rng *r;
const gsl_rng_type * T;
gsl_ran_discrete_t* uncoop_before_discrete, *uncoop_after_discrete;

struct array* nodes;
struct array* edges;
struct array* channels;

long n_dijkstra;

struct node* create_node(long id, long edges_size) {
  struct node* node;

  node = malloc(sizeof(struct node));
  node->id=id;
  node->open_edges = array_initialize(edges_size);
 
  node_index++;

  return node;
}

struct channel* create_channel(long id, long node1, long node2, uint32_t latency) {
  struct channel* channel;

  channel = malloc(sizeof(struct channel));
  channel->id = id;
  channel->node1 = node1;
  channel->node2 = node2;
  channel->latency = latency;
  channel->capacity = 0;
  channel->is_closed = 0;

  channel_index++;

  return channel;
}


struct edge* create_edge(long id, long channel_id, long counterparty, struct policy policy){
  struct edge* edge;

  edge = malloc(sizeof(struct edge));
  edge->id = id;
  edge->channel_id = channel_id;
  edge->counterparty = counterparty;
  edge->policy = policy;
  edge->balance = 0;
  edge->is_closed = 0;

  edge_index++;

  return edge;
}

struct payment* create_payment(long id, long sender, long receiver, uint64_t amount) {
  struct payment * p;

  p = malloc(sizeof(struct payment));
  p->id=id;
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

struct node* create_node_post_proc(long id) {
  struct node* node;

  node = malloc(sizeof(struct node));
  node->id=id;
  node->open_edges = array_initialize(5);
  node->ignored_edges = array_initialize(10);
  node->ignored_nodes = array_initialize(1);

  node_index++;

  return node;
}

struct channel* create_channel_post_proc(long id, long direction1, long direction2, long node1, long node2, uint64_t capacity, uint32_t latency) {
  struct channel* channel;

  channel = malloc(sizeof(struct channel));
  channel->id = id;
  channel->edge1 = direction1;
  channel->edge2 = direction2;
  channel->node1 = node1;
  channel->node2 = node2;
  channel->latency = latency;
  channel->capacity = capacity;
  channel->is_closed = 0;

  channel_index++;

  return channel;
}

struct edge* create_edge_post_proc(long id, long channel_id, long other_direction, long counterparty, uint64_t balance, struct policy policy){
  struct edge* edge;

  edge = malloc(sizeof(struct edge));
  edge->id = id;
  edge->channel_id = channel_id;
  edge->counterparty = counterparty;
  edge->other_edge_id = other_direction;
  edge->policy = policy;
  edge->balance = balance;
  edge->is_closed = 0;

  edge_index++;

  return edge;
}





/* double compute_gini() { */
/*   long i, j; */
/*   __uint128_t num=0, den=0; */
/*   __uint128_t difference; */
/*   struct channel *channeli, *channelj; */
/*   double gini; */

/*   for(i=0;i<channel_info_index; i++) { */
/*     channeli = array_get(channel_infos, i); */
/*     den += channeli->capacity; */
/*     for(j=0; j<channel_info_index; j++){ */
/*       if(i==j) continue; */
/*       channelj = array_get(channel_infos, j); */
/*       if(channeli->capacity > channelj->capacity) */
/*         difference = channeli->capacity - channelj->capacity; */
/*       else */
/*         difference = channelj->capacity - channeli->capacity; */
/*       num += difference; */
/*     } */
/*   } */

/*   den = 2*channel_info_index*den; */

/*   gini = num/(den*1.0); */

/*   return gini; */
/* } */

void initialize_topology_preproc(long n_nodes, long n_channels, double RWithholding, int gini, int sigma, long capacity_per_channel) {
  long i, j, node_idIndex, channel_idIndex, edge_idIndex, node1, node2, direction1, direction2, info;
  double Rwithholding_p[] = {1-RWithholding, RWithholding}, balance_p[] = {0.5, 0.5}, gini_p[3], min_htlcP[]={0.7, 0.2, 0.05, 0.05},coeff1, coeff2, mean=n_nodes/2 ;
  gsl_ran_discrete_t* Rwithholding_discrete, *balance_discrete, *gini_discrete, *min_htlcDiscrete;
  int *node_edges;
  uint32_t latency;
  uint64_t balance1, balance2, capacity, min_htlc;
  struct policy policy1, policy2;
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

  thresh1 = n_nodes*n_channels*coeff1;
  thresh2 = n_nodes*n_channels*coeff2;

  // NOTE: *5 instead of *n_channels if you want to test Gini
  funds_part = (capacity_per_channel/3)*n_nodes*n_channels;

//  printf("%ld, %ld\n", thresh1, thresh2);

  /* if(gini != 5) { */
  funds[0] = (funds_part)/thresh1;
  funds[1] = (funds_part)/(thresh2);
  funds[2] = (funds_part)/ (n_nodes*n_channels - (thresh1 + thresh2));

  csv_node = fopen("node.csv", "w");
  if(csv_node==NULL) {
    printf("ERROR cannot open file node.csv\n");
    return;
  }
  fprintf(csv_node, "id,withholds_r\n");

  csv_channel = fopen("channel.csv", "w");
  if(csv_channel==NULL) {
    printf("ERROR cannot open file channel.csv\n");
    return;
  }
  fprintf(csv_channel, "id,direction1,direction2,node1,node2,capacity,latency\n");

  csv_edge = fopen("edge.csv", "w");
  if(csv_edge==NULL) {
    printf("ERROR cannot open file edge.csv\n");
    return;
  }
  fprintf(csv_edge, "id,channel,other_direction,counterparty,balance,fee_base,fee_proportional,min_htlc,timelock\n");


  Rwithholding_discrete = gsl_ran_discrete_preproc(2, Rwithholding_p);
  balance_discrete = gsl_ran_discrete_preproc(2, balance_p);


  node_idIndex=0;
  for(i=0; i<n_nodes; i++){
    fprintf(csv_node, "%ld,%ld\n", node_idIndex, gsl_ran_discrete(r, Rwithholding_discrete));
    node_idIndex++;
  }

  node_edges = malloc(node_idIndex*sizeof(int));
  for(i=0;i<node_idIndex;i++) 
    node_edges[i] = 0;

  channel_idIndex = edge_idIndex= 0;
  for(i=0; i<node_idIndex; i++) {
    node1 = i;
    for(j=0; j<n_channels &&  (node_edges[node1] < n_channels); j++){

      do {
       if(j==0 && sigma!=-1) {
          node2 = mean + gsl_ran_gaussian(r, sigma);
          }
          else
            node2 = gsl_rng_uniform_int(r,node_idIndex);
      }while(node2==node1);

      if(node2<0 || node2>=node_idIndex) continue;

      if(sigma!=-1) {
        if(j!=0 && node_edges[node2]>=n_channels) continue;
      }
      else {
        if(node_edges[node2]>=n_channels) continue;
      }



      ++node_edges[node1];
      ++node_edges[node2];
      info = channel_idIndex;
      ++channel_idIndex;
      direction1 = edge_idIndex;
      ++edge_idIndex;
      direction2 = edge_idIndex;
      ++edge_idIndex;

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


      min_htlcDiscrete = gsl_ran_discrete_preproc(4, min_htlcP);


      policy1.fee_base = gsl_rng_uniform_int(r, MAXFEEBASE - MINFEEBASE) + MINFEEBASE;
      policy1.fee_proportional = (gsl_rng_uniform_int(r, MAXFEEPROP-MINFEEPROP)+MINFEEPROP);
      policy1.timelock = gsl_rng_uniform_int(r, MAXTIMELOCK-MINTIMELOCK)+MINTIMELOCK;
      policy1.min_htlc = gsl_pow_int(10, gsl_ran_discrete(r, min_htlcDiscrete));
      policy1.min_htlc = policy1.min_htlc == 1 ? 0 : policy1.min_htlc;
      policy2.fee_base = gsl_rng_uniform_int(r, MAXFEEBASE - MINFEEBASE) + MINFEEBASE;
      policy2.fee_proportional = (gsl_rng_uniform_int(r, MAXFEEPROP-MINFEEPROP)+MINFEEPROP);
      policy2.timelock = gsl_rng_uniform_int(r, MAXTIMELOCK-MINTIMELOCK)+MINTIMELOCK;
      policy2.min_htlc = gsl_pow_int(10, gsl_ran_discrete(r, min_htlcDiscrete));
      policy2.min_htlc = policy2.min_htlc == 1 ? 0 : policy2.min_htlc;

      fprintf(csv_channel, "%ld,%ld,%ld,%ld,%ld,%ld,%d\n", info, direction1, direction2, node1, node2, capacity, latency);

      fprintf(csv_edge, "%ld,%ld,%ld,%ld,%ld,%d,%d,%d,%d\n", direction1, info, direction2, node2, balance1, policy1.fee_base, policy1.fee_proportional, policy1.min_htlc, policy1.timelock);

      fprintf(csv_edge, "%ld,%ld,%ld,%ld,%ld,%d,%d,%d,%d\n", direction2, info, direction1, node1, balance2, policy2.fee_base, policy2.fee_proportional, policy2.min_htlc, policy2.timelock);

    }

  }


/*   double num = 0, den = 0; */
/*   for(i=0; i<node_idIndex; i++) { */
/*     //    if(node_channels[i]>5) printf("%ld, %d\n", i, node_channels[i]); */
/*     // else { */
/*     if(node_channels[i]>5) */
/*       printf("%ld, %d\n", i, node_channels[i]); */
/*     den++; */
/*     num += node_channels[i]; */
/*       //} */
/*   } */

/*   printf("mean channels: %lf\n", num/den); */

/* exit(-1); */

  fclose(csv_node);
  fclose(csv_channel);
  fclose(csv_edge);


}


void create_topology_from_csv(unsigned int is_preproc) {
  char row[256], edge_file[64], info_file[64], node_file[64];
  struct node* node, *node1, *node2;
  long id, direction1, direction2, node_id1, node_id2, channel_id, other_direction, counterparty;
  struct policy policy;
  int withholds_r;
  uint32_t latency;
  uint64_t capacity, balance;
  struct channel* channel;
  struct edge* edge;

  if(is_preproc) {
    strcpy(edge_file, "edge.csv");
    strcpy(info_file, "channel.csv");
    strcpy(node_file, "node.csv");
  }
  else {
    strcpy(edge_file, "edge_ln.csv");
    strcpy(info_file, "channel_ln.csv");
    strcpy(node_file, "node_ln.csv");
    }


  csv_node = fopen(node_file, "r");
  if(csv_node==NULL) {
    printf("ERROR cannot open file node.csv\n");
    return;
  }

  fgets(row, 256, csv_node);
  while(fgets(row, 256, csv_node)!=NULL) {
    sscanf(row, "%ld,%d", &id, &withholds_r);
    node = create_node_post_proc(id);
    nodes = array_insert(nodes, node);
  }

  fclose(csv_node);

  csv_channel = fopen(info_file, "r");
  if(csv_channel==NULL) {
    printf("ERROR cannot open file %s\n", info_file);
    return;
  }

  fgets(row, 256, csv_channel);
  while(fgets(row, 256, csv_channel)!=NULL) {
    sscanf(row, "%ld,%ld,%ld,%ld,%ld,%ld,%d", &id, &direction1, &direction2, &node_id1, &node_id2, &capacity, &latency);
    channel = create_channel_post_proc(id, direction1, direction2, node_id1, node_id2, capacity, latency);
    channels = array_insert(channels, channel);
    node1 = array_get(nodes, node_id1);
    node1->open_edges = array_insert(node1->open_edges, &(channel->edge1));
    node2 = array_get(nodes, node_id2);
    node2->open_edges = array_insert(node2->open_edges, &(channel->edge2));
  }

  fclose(csv_channel);

  csv_edge = fopen(edge_file, "r");
  if(csv_edge==NULL) {
    printf("ERROR cannot open file %s\n", edge_file);
    return;
  }

  fgets(row, 256, csv_edge);
  while(fgets(row, 256, csv_edge)!=NULL) {
    sscanf(row, "%ld,%ld,%ld,%ld,%ld,%d,%d,%d,%hd", &id, &channel_id, &other_direction, &counterparty, &balance, &policy.fee_base, &policy.fee_proportional, &policy.min_htlc, &policy.timelock);
    edge = create_edge_post_proc(id, channel_id, other_direction, counterparty, balance, policy);
    edges = array_insert(edges, edge);
  }

  fclose(csv_edge);

}



void initialize_protocol_data(long n_nodes, long n_channels, double p_uncoop_before, double p_uncoop_after, double RWithholding, int gini, int sigma, long capacity_per_channel, unsigned int is_preproc) {
  double before_p[] = {p_uncoop_before, 1-p_uncoop_before};
  double after_p[] = {p_uncoop_after, 1-p_uncoop_after};

  gsl_rng_env_setup();
  T = gsl_rng_default;
  r = gsl_rng_alloc (T);

  uncoop_before_discrete = gsl_ran_discrete_preproc(2, before_p);
  uncoop_after_discrete = gsl_ran_discrete_preproc(2, after_p);

  edge_index = node_index = channel_index = payment_index = 0;

  n_dijkstra = 0;

  nodes = array_initialize(n_nodes);
  edges = array_initialize(n_channels*n_nodes*2);
  channels = array_initialize(n_channels*n_nodes);


  if(is_preproc)
    initialize_topology_preproc(n_nodes, n_channels, RWithholding, gini, sigma, capacity_per_channel);

  create_topology_from_csv(is_preproc);



}


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
  return gsl_ran_discrete(r, uncoop_before_discrete);
}

int is_cooperative_after_lock() {
  return gsl_ran_discrete(r, uncoop_after_discrete);
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
        n_dijkstra++;
      }
  }
  else {
    path_hops = dijkstra(payment->sender, payment->receiver, payment->amount, node->ignored_nodes,
                        node->ignored_edges);
    n_dijkstra++;
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
  send_event = create_event(event_index, next_event_time, SENDPAYMENT, payment->sender, event->payment_id );
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
    next_event = create_event(event_index, next_event_time, FINDROUTE, event->node_id, event->payment_id );
    events = heap_insert(events, next_event, compare_event);
    return;
  }


  if(first_route_hop->amount_to_forward > next_edge->balance) {
    printf("Not enough balance in edge %ld\n", next_edge->id);
    add_ignored(payment->sender, next_edge->id);
    next_event_time = simulator_time;
    next_event = create_event(event_index, next_event_time, FINDROUTE, event->node_id, event->payment_id );
    events = heap_insert(events, next_event, compare_event);
    return;
  }

  next_edge->balance -= first_route_hop->amount_to_forward;


  event_type = first_route_hop->path_hop->receiver == payment->receiver ? RECEIVEPAYMENT : FORWARDPAYMENT;
  next_event_time = simulator_time + channel->latency;
  next_event = create_event(event_index, next_event_time, event_type, first_route_hop->path_hop->receiver, event->payment_id );
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
    next_event = create_event(event_index, next_event_time, event_type, prev_node_id, event->payment_id);
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
    next_event = create_event(event_index, next_event_time, event_type, prev_node_id, event->payment_id );
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
    next_event = create_event(event_index, next_event_time, event_type, prev_node_id, event->payment_id );
    events = heap_insert(events, next_event, compare_event);
    return;
  }
  next_edge->balance -= next_route_hop->amount_to_forward;


  next_node_id = next_route_hop->path_hop->receiver;
  event_type = next_node_id == payment->receiver ? RECEIVEPAYMENT : FORWARDPAYMENT;
  next_event_time = simulator_time + next_channel->latency;
  next_event = create_event(event_index, next_event_time, event_type, next_node_id, event->payment_id );
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
    next_event = create_event(event_index, next_event_time, event_type, prev_node_id, event->payment_id );
    events = heap_insert(events, next_event, compare_event);
    return;
  }

  payment->is_success = 1;

  prev_node_id = last_route_hop->path_hop->sender;
  event_type = prev_node_id == payment->sender ? RECEIVESUCCESS : FORWARDSUCCESS;
  next_event_time = simulator_time + channel->latency;
  next_event = create_event(event_index, next_event_time, event_type, prev_node_id, event->payment_id);
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
    next_event = create_event(event_index, next_event_time, event_type, prev_node_id, event->payment_id);
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
  next_event = create_event(event_index, next_event_time, event_type, prev_node_id, event->payment_id);
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
  next_event = create_event(event_index, next_event_time, event_type, prev_node_id, event->payment_id);
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
  next_event = create_event(event_index, next_event_time, FINDROUTE, payment->sender, payment->id);
  events = heap_insert(events, next_event, compare_event);
}

