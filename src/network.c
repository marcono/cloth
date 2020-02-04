#include <string.h>

#include "gsl/gsl_math.h"
#include "../include/network.h"
#include "../include/array.h"

gsl_ran_discrete_t* uncoop_before_discrete, *uncoop_after_discrete;

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


void initialize_network_preproc(struct network_params net_params) {
  long i, j, node_idIndex, channel_idIndex, edge_idIndex, node1, node2, direction1, direction2, info;
  double Rwithholding_p[] = {1-net_params.RWithholding, net_params.RWithholding}, balance_p[] = {0.5, 0.5}, gini_p[3], min_htlcP[]={0.7, 0.2, 0.05, 0.05},coeff1, coeff2, mean=net_params.n_nodes/2 ;
  gsl_ran_discrete_t* Rwithholding_discrete, *balance_discrete, *gini_discrete, *min_htlcDiscrete;
  int *node_edges;
  uint32_t latency;
  uint64_t balance1, balance2, capacity;
  struct policy policy1, policy2;
  long thresh1, thresh2;
  uint64_t funds[3], funds_part;

  switch(net_params.gini) {
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

  thresh1 = (net_params.n_nodes)*(net_params.n_channels)*coeff1;
  thresh2 = (net_params.n_nodes)*(net_params.n_channels)*coeff2;

  // NOTE: *5 instead of *n_channels if you want to test Gini
  funds_part = (net_params.capacity_per_channel/3)*(net_params.n_nodes)*(net_params.n_channels);

//  printf("%ld, %ld\n", thresh1, thresh2);

  /* if(gini != 5) { */
  funds[0] = (funds_part)/thresh1;
  funds[1] = (funds_part)/(thresh2);
  funds[2] = (funds_part)/ ((net_params.n_nodes)*(net_params.n_channels) - (thresh1 + thresh2));

  csv_node = fopen("nodes.csv", "w");
  if(csv_node==NULL) {
    printf("ERROR cannot open file nodes.csv\n");
    exit(-1);
  }
  fprintf(csv_node, "id,withholds_r\n");

  csv_channel = fopen("channels.csv", "w");
  if(csv_channel==NULL) {
    printf("ERROR cannot open file channels.csv\n");
    exit(-1);
  }
  fprintf(csv_channel, "id,direction1,direction2,node1,node2,capacity,latency\n");

  csv_edge = fopen("edges.csv", "w");
  if(csv_edge==NULL) {
    printf("ERROR cannot open file edges.csv\n");
    exit(-1);
  }
  fprintf(csv_edge, "id,channel,other_direction,counterparty,balance,fee_base,fee_proportional,min_htlc,timelock\n");


  Rwithholding_discrete = gsl_ran_discrete_preproc(2, Rwithholding_p);
  balance_discrete = gsl_ran_discrete_preproc(2, balance_p);


  node_idIndex=0;
  for(i=0; i<net_params.n_nodes; i++){
    fprintf(csv_node, "%ld,%ld\n", node_idIndex, gsl_ran_discrete(random_generator, Rwithholding_discrete));
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

      capacity = funds[gsl_ran_discrete(random_generator, gini_discrete)];


      double balance_sigma=2.0, fraction;
      int gauss;

      gauss=gsl_ran_gaussian(random_generator, balance_sigma);

      if(gsl_ran_discrete(random_generator, balance_discrete))
        gauss = 2+gauss;
      else
        gauss = 7+gauss;

      if(gauss>10) gauss=10;
      if(gauss<0) gauss = 0;

      fraction = gauss/10.0;
      balance1 = fraction*capacity;
      balance2 = capacity - balance1;


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


struct network* generate_network_from_file(struct network_params net_params, unsigned int is_preproc) {
  char row[256], edge_file[64], info_file[64], node_file[64];
  struct node* node, *node1, *node2;
  long id, direction1, direction2, node_id1, node_id2, channel_id, other_direction, counterparty;
  struct policy policy;
  int withholds_r;
  uint32_t latency;
  uint64_t capacity, balance;
  struct channel* channel;
  struct edge* edge;
  struct network* network;

  if(is_preproc) {
    strcpy(edge_file, "edges.csv");
    strcpy(info_file, "channels.csv");
    strcpy(node_file, "nodes.csv");
  }
  else {
    strcpy(edge_file, "edges_ln.csv");
    strcpy(info_file, "channels_ln.csv");
    strcpy(node_file, "nodes_ln.csv");
    }

  network = (struct network*) malloc(sizeof(struct network));
  network->nodes = array_initialize(net_params.n_nodes);
  network->channels = array_initialize(net_params.n_channels*net_params.n_nodes);
  network->edges = array_initialize(net_params.n_channels*net_params.n_nodes*2);

  csv_node = fopen(node_file, "r");
  if(csv_node==NULL) {
    printf("ERROR cannot open file %s\n", node_file);
    exit(-1);
  }

  fgets(row, 256, csv_node);
  while(fgets(row, 256, csv_node)!=NULL) {
    sscanf(row, "%ld,%d", &id, &withholds_r);
    node = new_node(id);
    network->nodes = array_insert(network->nodes, node);
  }
  fclose(csv_node);

  csv_channel = fopen(info_file, "r");
  if(csv_channel==NULL) {
    printf("ERROR cannot open file %s\n", info_file);
    exit(-1);
  }

  fgets(row, 256, csv_channel);
  while(fgets(row, 256, csv_channel)!=NULL) {
    sscanf(row, "%ld,%ld,%ld,%ld,%ld,%ld,%d", &id, &direction1, &direction2, &node_id1, &node_id2, &capacity, &latency);
    channel = new_channel(id, direction1, direction2, node_id1, node_id2, capacity, latency);
    network->channels = array_insert(network->channels, channel);
    node1 = array_get(network->nodes, node_id1);
    node1->open_edges = array_insert(node1->open_edges, &(channel->edge1));
    node2 = array_get(network->nodes, node_id2);
    node2->open_edges = array_insert(node2->open_edges, &(channel->edge2));
  }

  fclose(csv_channel);

  csv_edge = fopen(edge_file, "r");
  if(csv_edge==NULL) {
    printf("ERROR cannot open file %s\n", edge_file);
    exit(-1);
  }

  fgets(row, 256, csv_edge);
  while(fgets(row, 256, csv_edge)!=NULL) {
    sscanf(row, "%ld,%ld,%ld,%ld,%ld,%d,%d,%d,%hd", &id, &channel_id, &other_direction, &counterparty, &balance, &policy.fee_base, &policy.fee_proportional, &policy.min_htlc, &policy.timelock);
    edge = new_edge(id, channel_id, other_direction, counterparty, balance, policy);
    network->edges = array_insert(network->edges, edge);
  }

  fclose(csv_edge);

  return network;


}



struct network* initialize_network(struct network_params net_params, unsigned int is_preproc) {
  double before_p[] = {net_params.p_uncoop_before_lock, 1-net_params.p_uncoop_before_lock};
  double after_p[] = {net_params.p_uncoop_after_lock, 1-net_params.p_uncoop_after_lock};

  uncoop_before_discrete = gsl_ran_discrete_preproc(2, before_p);
  uncoop_after_discrete = gsl_ran_discrete_preproc(2, after_p);

  if(is_preproc)
    initialize_network_preproc(net_params);

  return generate_network_from_file(net_params, is_preproc);
}
