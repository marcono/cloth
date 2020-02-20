#include <string.h>

#include "gsl/gsl_math.h"
#include "../include/network.h"
#include "../include/array.h"


struct node* new_node(long id) {
  struct node* node;

  node = malloc(sizeof(struct node));
  node->id=id;
  node->open_edges = array_initialize(5);
  node->ignored_edges = array_initialize(10);
  node->ignored_nodes = array_initialize(1);

  return node;
}

struct channel* new_channel(long id, long direction1, long direction2, long node1, long node2, uint64_t capacity, uint32_t latency) {
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

  return channel;
}

struct edge* new_edge(long id, long channel_id, long other_direction, long counterparty, uint64_t balance, struct policy policy){
  struct edge* edge;

  edge = malloc(sizeof(struct edge));
  edge->id = id;
  edge->channel_id = channel_id;
  edge->counterparty = counterparty;
  edge->other_edge_id = other_direction;
  edge->policy = policy;
  edge->balance = balance;
  edge->is_closed = 0;

  return edge;
}


void initialize_random_network(struct network_params net_params, gsl_rng *random_generator) {
  long i, j, node_idIndex, channel_idIndex, edge_idIndex, node1, node2, direction1, direction2, info;
  double min_htlcP[]={0.7, 0.2, 0.05, 0.05}, mean=net_params.n_nodes/2, fraction_capacity;
  gsl_ran_discrete_t  *min_htlcDiscrete;
  int *node_edges;
  uint32_t latency;
  uint64_t balance1, balance2, capacity;
  struct policy policy1, policy2;
  FILE *nodes_file, *channels_file, *edges_file;


  nodes_file = fopen("nodes.csv", "w");
  if(nodes_file==NULL) {
    printf("ERROR cannot open file nodes.csv\n");
    exit(-1);
  }
  fprintf(nodes_file, "id\n");

  channels_file = fopen("channels.csv", "w");
  if(channels_file==NULL) {
    printf("ERROR cannot open file channels.csv\n");
    exit(-1);
  }
  fprintf(channels_file, "id,direction1,direction2,node1,node2,capacity,latency\n");

  edges_file = fopen("edges.csv", "w");
  if(edges_file==NULL) {
    printf("ERROR cannot open file edges.csv\n");
    exit(-1);
  }
  fprintf(edges_file, "id,channel,other_direction,counterparty,balance,fee_base,fee_proportional,min_htlc,timelock\n");



  node_idIndex=0;
  for(i=0; i<net_params.n_nodes; i++){
    fprintf(nodes_file, "%ld\n", node_idIndex);
    node_idIndex++;
  }

  node_edges = malloc(node_idIndex*sizeof(int));
  for(i=0;i<node_idIndex;i++) 
    node_edges[i] = 0;

  channel_idIndex = edge_idIndex= 0;
  for(i=0; i<node_idIndex; i++) {
    node1 = i;
    for(j=0; j<net_params.n_channels &&  (node_edges[node1] < net_params.n_channels); j++){

      do {
       if(j==0 && net_params.sigma_topology!=-1) {
          node2 = mean + gsl_ran_gaussian(random_generator, net_params.sigma_topology);
          }
          else
            node2 = gsl_rng_uniform_int(random_generator,node_idIndex);
      }while(node2==node1);

      if(node2<0 || node2>=node_idIndex) continue;

      if(net_params.sigma_topology!=-1) {
        if(j!=0 && node_edges[node2]>=net_params.n_channels) continue;
      }
      else {
        if(node_edges[node2]>=net_params.n_channels) continue;
      }



      ++node_edges[node1];
      ++node_edges[node2];
      info = channel_idIndex;
      ++channel_idIndex;
      direction1 = edge_idIndex;
      ++edge_idIndex;
      direction2 = edge_idIndex;
      ++edge_idIndex;

      latency = gsl_rng_uniform_int(random_generator, MAXLATENCY - MINLATENCY) + MINLATENCY;

      capacity = net_params.capacity_per_channel + gsl_ran_ugaussian(random_generator); 

      fraction_capacity = (5 + gsl_ran_ugaussian(random_generator))/10; //a number between 0.1 and 1.0, mean 0.5
      balance1 = fraction_capacity*((double) capacity);
      balance2 = capacity - balance1;

      //multiplied by 1000 to convert satoshi to millisatoshi
      capacity*=1000;
      balance1*=1000;
      balance2*=1000;


      min_htlcDiscrete = gsl_ran_discrete_preproc(4, min_htlcP);


      policy1.fee_base = gsl_rng_uniform_int(random_generator, MAXFEEBASE - MINFEEBASE) + MINFEEBASE;
      policy1.fee_proportional = (gsl_rng_uniform_int(random_generator, MAXFEEPROP-MINFEEPROP)+MINFEEPROP);
      policy1.timelock = gsl_rng_uniform_int(random_generator, MAXTIMELOCK-MINTIMELOCK)+MINTIMELOCK;
      policy1.min_htlc = gsl_pow_int(10, gsl_ran_discrete(random_generator, min_htlcDiscrete));
      policy1.min_htlc = policy1.min_htlc == 1 ? 0 : policy1.min_htlc;
      policy2.fee_base = gsl_rng_uniform_int(random_generator, MAXFEEBASE - MINFEEBASE) + MINFEEBASE;
      policy2.fee_proportional = (gsl_rng_uniform_int(random_generator, MAXFEEPROP-MINFEEPROP)+MINFEEPROP);
      policy2.timelock = gsl_rng_uniform_int(random_generator, MAXTIMELOCK-MINTIMELOCK)+MINTIMELOCK;
      policy2.min_htlc = gsl_pow_int(10, gsl_ran_discrete(random_generator, min_htlcDiscrete));
      policy2.min_htlc = policy2.min_htlc == 1 ? 0 : policy2.min_htlc;

      fprintf(channels_file, "%ld,%ld,%ld,%ld,%ld,%ld,%d\n", info, direction1, direction2, node1, node2, capacity, latency);

      fprintf(edges_file, "%ld,%ld,%ld,%ld,%ld,%d,%d,%d,%d\n", direction1, info, direction2, node2, balance1, policy1.fee_base, policy1.fee_proportional, policy1.min_htlc, policy1.timelock);

      fprintf(edges_file, "%ld,%ld,%ld,%ld,%ld,%d,%d,%d,%d\n", direction2, info, direction1, node1, balance2, policy2.fee_base, policy2.fee_proportional, policy2.min_htlc, policy2.timelock);

    }

  }

  fclose(nodes_file);
  fclose(channels_file);
  fclose(edges_file);

}


struct network* generate_network(struct network_params net_params) {
  char row[256], edges_filename[256], channels_filename[256], nodes_filename[256];
  struct node* node, *node1, *node2;
  long id, direction1, direction2, node_id1, node_id2, channel_id, other_direction, counterparty;
  struct policy policy;
  uint32_t latency;
  uint64_t capacity, balance;
  struct channel* channel;
  struct edge* edge;
  struct network* network;
  FILE *nodes_file, *channels_file, *edges_file;
  double faulty_prob[2];

  if(!(net_params.network_from_file)) {
    strcpy(edges_filename, "edges.csv");
    strcpy(channels_filename, "channels.csv");
    strcpy(nodes_filename, "nodes.csv");
  }
  else {
    strcpy(edges_filename, net_params.edges_filename);
    strcpy(channels_filename, net_params.channels_filename);
    strcpy(nodes_filename, net_params.nodes_filename);
    }

  nodes_file = fopen(nodes_filename, "r");
  if(nodes_file==NULL) {
    fprintf(stderr, "ERROR: cannot open file <%s>\n", nodes_filename);
    exit(-1);
  }
  channels_file = fopen(channels_filename, "r");
  if(channels_file==NULL) {
    fprintf(stderr, "ERROR: cannot open file <%s>\n", channels_filename);
    exit(-1);
  }
  edges_file = fopen(edges_filename, "r");
  if(edges_file==NULL) {
    fprintf(stderr, "ERROR: cannot open file <%s>\n", edges_filename);
    exit(-1);
  }

  network = (struct network*) malloc(sizeof(struct network));
  network->nodes = array_initialize(1000);
  network->channels = array_initialize(1000);
  network->edges = array_initialize(2000);

  faulty_prob[0] = 1-net_params.faulty_node_prob;
  faulty_prob[1] = net_params.faulty_node_prob;
  network->faulty_node_prob = gsl_ran_discrete_preproc(2, faulty_prob);

  fgets(row, 256, nodes_file);
  while(fgets(row, 256, nodes_file)!=NULL) {
    sscanf(row, "%ld", &id);
    printf("node_id: %ld\n",id);
    node = new_node(id);
    network->nodes = array_insert(network->nodes, node);
  }
  fclose(nodes_file);

  fgets(row, 256, channels_file);
  while(fgets(row, 256, channels_file)!=NULL) {
    sscanf(row, "%ld,%ld,%ld,%ld,%ld,%ld,%d", &id, &direction1, &direction2, &node_id1, &node_id2, &capacity, &latency);
    channel = new_channel(id, direction1, direction2, node_id1, node_id2, capacity, latency);
    network->channels = array_insert(network->channels, channel);
    node1 = array_get(network->nodes, node_id1);
    node1->open_edges = array_insert(node1->open_edges, &(channel->edge1));
    node2 = array_get(network->nodes, node_id2);
    node2->open_edges = array_insert(node2->open_edges, &(channel->edge2));
  }
  fclose(channels_file);


  fgets(row, 256, edges_file);
  while(fgets(row, 256, edges_file)!=NULL) {
    sscanf(row, "%ld,%ld,%ld,%ld,%ld,%d,%d,%d,%hd", &id, &channel_id, &other_direction, &counterparty, &balance, &policy.fee_base, &policy.fee_proportional, &policy.min_htlc, &policy.timelock);
    edge = new_edge(id, channel_id, other_direction, counterparty, balance, policy);
    network->edges = array_insert(network->edges, edge);
  }
  fclose(edges_file);

  return network;
}


struct network* initialize_network(struct network_params net_params, gsl_rng* random_generator) {

  if(!(net_params.network_from_file))
    initialize_random_network(net_params, random_generator);

  return  generate_network(net_params);
}
