#include <string.h>

#include <gsl/gsl_math.h>
#include <gsl/gsl_rng.h>
#include "../include/network.h"
#include "../include/array.h"

struct node* new_node(long id) {
  struct node* node;

  node = malloc(sizeof(struct node));
  node->id=id;
  node->open_edges = array_initialize(10);
  node->results = NULL;
  node->explored = 0;

  return node;
}

struct channel* new_channel(long id, long direction1, long direction2, long node1, long node2, uint64_t capacity) {
  struct channel* channel;

  channel = malloc(sizeof(struct channel));
  channel->id = id;
  channel->edge1 = direction1;
  channel->edge2 = direction2;
  channel->node1 = node1;
  channel->node2 = node2;
  channel->capacity = capacity;
  channel->is_closed = 0;

  return channel;
}

struct edge* new_edge(long id, long channel_id, long counter_edge_id, long from_node_id, long to_node_id, uint64_t balance, struct policy policy){
  struct edge* edge;

  edge = malloc(sizeof(struct edge));
  edge->id = id;
  edge->channel_id = channel_id;
  edge->from_node_id = from_node_id;
  edge->to_node_id = to_node_id;
  edge->counter_edge_id = counter_edge_id;
  edge->policy = policy;
  edge->balance = balance;
  edge->is_closed = 0;
  edge->tot_flows = 0;

  return edge;
}


void write_network_files(struct network* network){
  FILE* nodes_output_file, *edges_output_file, *channels_output_file;
  long i;
  struct node* node;
  struct channel* channel;
  struct edge* edge;

  nodes_output_file = fopen("nodes.csv", "w");
  if(nodes_output_file==NULL) {
    fprintf(stderr, "ERROR: cannot open file <%s>\n", "nodes.csv");
    exit(-1);
  }
  fprintf(nodes_output_file, "id\n");
  channels_output_file = fopen("channels.csv", "w");
  if(channels_output_file==NULL) {
    fprintf(stderr, "ERROR: cannot open file <%s>\n", "channels.csv");
    exit(-1);
  }
  fprintf(channels_output_file, "id,edge1_id,edge2_id,node1_id,node2_id,capacity,latency\n");
  edges_output_file = fopen("edges.csv", "w");
  if(edges_output_file==NULL) {
    fprintf(stderr, "ERROR: cannot open file <%s>\n", "edges.csv");
    exit(-1);
  }
  fprintf(edges_output_file, "id,channel_id,counter_edge_id,from_node_id,to_node_id,balance,fee_base,fee_proportional,min_htlc,timelock\n");

  for(i=0; i<array_len(network->nodes); i++){
    node = array_get(network->nodes, i);
    fprintf(nodes_output_file, "%ld\n", node->id);
  }

  for(i=0; i<array_len(network->channels); i++){
    channel = array_get(network->channels, i);
    fprintf(channels_output_file, "%ld,%ld,%ld,%ld,%ld,%ld\n", channel->id, channel->edge1, channel->edge2, channel->node1, channel->node2, channel->capacity);
  }

  for(i=0; i<array_len(network->edges); i++){
    edge = array_get(network->edges, i);
    fprintf(edges_output_file, "%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%d\n", edge->id, edge->channel_id, edge->counter_edge_id, edge->from_node_id, edge->to_node_id, edge->balance, (edge->policy).fee_base, (edge->policy).fee_proportional, (edge->policy).min_htlc, (edge->policy).timelock);
  }

  fclose(nodes_output_file);
  fclose(edges_output_file);
  fclose(channels_output_file);
}


void update_probability_per_node(double *probability_per_node, int *channels_per_node, long n_nodes, long node1_id, long node2_id, long tot_channels){
  long i;
  channels_per_node[node1_id] += 1;
  channels_per_node[node2_id] += 1;
  for(i=0; i<n_nodes; i++)
    probability_per_node[i] = ((double)channels_per_node[i])/tot_channels;
}


void generate_random_channel(long channel_id, long edge1_id, long edge2_id, long node1_id, long node2_id,  uint64_t channel_capacity, struct network* network, gsl_rng*random_generator) {
  uint64_t capacity, edge1_balance, edge2_balance;
  struct policy edge1_policy, edge2_policy;
  double min_htlcP[]={0.7, 0.2, 0.05, 0.05}, fraction_capacity;
  gsl_ran_discrete_t* min_htlc_discrete;
  struct channel* channel;
  struct edge* edge1, *edge2;
  struct node* node;

  capacity = channel_capacity + gsl_ran_ugaussian(random_generator); //TODO: check that it doesn't produce negative numbers fow small values of channel_capacity
  channel = new_channel(channel_id, edge1_id, edge2_id, node1_id, node2_id, capacity*1000);

  //  fraction_capacity = (5 + gsl_ran_ugaussian(random_generator))/10; //a number between 0.1 and 1.0, mean 0.5
  fraction_capacity = gsl_rng_uniform(random_generator);
  edge1_balance = fraction_capacity*((double) capacity);
  edge2_balance = capacity - edge1_balance;
  //multiplied by 1000 to convert satoshi to millisatoshi
  edge1_balance*=1000;
  edge2_balance*=1000;

  min_htlc_discrete = gsl_ran_discrete_preproc(4, min_htlcP);
  edge1_policy.fee_base = gsl_rng_uniform_int(random_generator, MAXFEEBASE - MINFEEBASE) + MINFEEBASE;
  edge1_policy.fee_proportional = (gsl_rng_uniform_int(random_generator, MAXFEEPROP-MINFEEPROP)+MINFEEPROP);
  edge1_policy.timelock = gsl_rng_uniform_int(random_generator, MAXTIMELOCK-MINTIMELOCK)+MINTIMELOCK;
  edge1_policy.min_htlc = gsl_pow_int(10, gsl_ran_discrete(random_generator, min_htlc_discrete));
  edge1_policy.min_htlc = edge1_policy.min_htlc == 1 ? 0 : edge1_policy.min_htlc;
  edge2_policy.fee_base = gsl_rng_uniform_int(random_generator, MAXFEEBASE - MINFEEBASE) + MINFEEBASE;
  edge2_policy.fee_proportional = (gsl_rng_uniform_int(random_generator, MAXFEEPROP-MINFEEPROP)+MINFEEPROP);
  edge2_policy.timelock = gsl_rng_uniform_int(random_generator, MAXTIMELOCK-MINTIMELOCK)+MINTIMELOCK;
  edge2_policy.min_htlc = gsl_pow_int(10, gsl_ran_discrete(random_generator, min_htlc_discrete));
  edge2_policy.min_htlc = edge2_policy.min_htlc == 1 ? 0 : edge2_policy.min_htlc;

  edge1 = new_edge(edge1_id, channel_id, edge2_id, node1_id, node2_id, edge1_balance, edge1_policy);
  edge2 = new_edge(edge2_id, channel_id, edge1_id, node2_id, node1_id, edge2_balance, edge2_policy);

  network->channels = array_insert(network->channels, channel);
  network->edges = array_insert(network->edges, edge1);
  network->edges = array_insert(network->edges, edge2);

  node = array_get(network->nodes, node1_id);
  node->open_edges = array_insert(node->open_edges, &(edge1->id));
  node = array_get(network->nodes, node2_id);
  node->open_edges = array_insert(node->open_edges, &(edge2->id));
}


struct network* generate_random_network(struct network_params net_params, gsl_rng* random_generator){
  FILE* nodes_input_file, *channels_input_file;
  char row[256];
  long node_id_counter=0, id, node1_id, node2_id, edge1_id, edge2_id, channel_id_counter=0, tot_nodes, i, tot_channels, node_to_connect_id, edge_id_counter=0, j;
  double *probability_per_node;
  int *channels_per_node;
  struct network* network;
  struct node* node;
  gsl_ran_discrete_t* connection_probability;

  nodes_input_file = fopen("nodes_ln.csv", "r");
  if(nodes_input_file==NULL) {
    fprintf(stderr, "ERROR: cannot open file <%s>\n", "nodes_ln.csv");
    exit(-1);
  }
  channels_input_file = fopen("channels_ln.csv", "r");
  if(channels_input_file==NULL) {
    fprintf(stderr, "ERROR: cannot open file <%s>\n", "channels_ln.csv");
    exit(-1);
  }

  network = (struct network*) malloc(sizeof(struct network));
  network->nodes = array_initialize(1000);
  network->channels = array_initialize(1000);
  network->edges = array_initialize(2000);

  fgets(row, 256, nodes_input_file);
  while(fgets(row, 256, nodes_input_file)!=NULL) {
    sscanf(row, "%ld,%*d", &id);
    node = new_node(id);
    network->nodes = array_insert(network->nodes, node);
    node_id_counter++;
  }

  tot_nodes = node_id_counter + net_params.n_nodes;
  channels_per_node = malloc(sizeof(int)*(tot_nodes));
  for(i = 0; i < tot_nodes; i++){
    channels_per_node[i] = 0;
  }

  fgets(row, 256, channels_input_file);
  while(fgets(row, 256, channels_input_file)!=NULL) {
    sscanf(row, "%ld,%ld,%ld,%ld,%ld,%*d,%*d", &id, &edge1_id, &edge2_id, &node1_id, &node2_id);
    generate_random_channel(id, edge1_id, edge2_id, node1_id, node2_id, net_params.capacity_per_channel, network, random_generator);
    channels_per_node[node1_id] += 1;
    channels_per_node[node2_id] += 1;
    ++channel_id_counter;
    edge_id_counter+=2;
  }

  tot_channels = channel_id_counter;
  probability_per_node = malloc(sizeof(double)*tot_nodes);
  for(i=0; i<tot_nodes; i++){
    probability_per_node[i] = ((double)channels_per_node[i])/tot_channels;
  }

  for(i=0; i<net_params.n_nodes; i++){
    node = new_node(node_id_counter);
    network->nodes = array_insert(network->nodes, node);
    for(j=0; j<net_params.n_channels; j++){
      connection_probability = gsl_ran_discrete_preproc(node_id_counter, probability_per_node);
      node_to_connect_id = gsl_ran_discrete(random_generator, connection_probability);
      generate_random_channel(channel_id_counter, edge_id_counter, edge_id_counter+1, node->id, node_to_connect_id, net_params.capacity_per_channel, network, random_generator);
      channel_id_counter++;
      edge_id_counter += 2;
      update_probability_per_node(probability_per_node, channels_per_node, tot_nodes, node->id, node_to_connect_id, channel_id_counter);
    }
    ++node_id_counter;
  }

  fclose(nodes_input_file);
  fclose(channels_input_file);

  write_network_files(network);

  return network;
}


struct network* generate_network_from_files(char nodes_filename[256], char channels_filename[256], char edges_filename[256]) {
  char row[2048];
  struct node* node;
  long id, direction1, direction2, node_id1, node_id2, channel_id, other_direction;
  struct policy policy;
  uint64_t capacity, balance;
  struct channel* channel;
  struct edge* edge;
  struct network* network;
  FILE *nodes_file, *channels_file, *edges_file;

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

  fgets(row, 2048, nodes_file);
  while(fgets(row, 2048, nodes_file)!=NULL) {
    sscanf(row, "%ld", &id);
    node = new_node(id);
    network->nodes = array_insert(network->nodes, node);
  }
  fclose(nodes_file);

  fgets(row, 2048, channels_file);
  while(fgets(row, 2048, channels_file)!=NULL) {
    sscanf(row, "%ld,%ld,%ld,%ld,%ld,%ld", &id, &direction1, &direction2, &node_id1, &node_id2, &capacity);
    channel = new_channel(id, direction1, direction2, node_id1, node_id2, capacity);
    network->channels = array_insert(network->channels, channel);
  }
  fclose(channels_file);


  fgets(row, 2048, edges_file);
  while(fgets(row, 2048, edges_file)!=NULL) {
    sscanf(row, "%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%d", &id, &channel_id, &other_direction, &node_id1, &node_id2, &balance, &policy.fee_base, &policy.fee_proportional, &policy.min_htlc, &policy.timelock);
    edge = new_edge(id, channel_id, other_direction, node_id1, node_id2, balance, policy);
    network->edges = array_insert(network->edges, edge);
    node = array_get(network->nodes, node_id1);
    node->open_edges = array_insert(node->open_edges, &(edge->id));
  }
  fclose(edges_file);

  return network;
}


struct network* initialize_network(struct network_params net_params, gsl_rng* random_generator) {
  struct network* network;
  double faulty_prob[2];
  long n_nodes;
  long i, j;
  struct node* node;

  if(net_params.network_from_file)
    network = generate_network_from_files(net_params.nodes_filename, net_params.channels_filename, net_params.edges_filename);
  else
   network = generate_random_network(net_params, random_generator);

  faulty_prob[0] = 1-net_params.faulty_node_prob;
  faulty_prob[1] = net_params.faulty_node_prob;
  network->faulty_node_prob = gsl_ran_discrete_preproc(2, faulty_prob);

  n_nodes = array_len(network->nodes);
  for(i=0; i<n_nodes; i++){
    node = array_get(network->nodes, i);
    node->results = (struct element**) malloc(n_nodes*sizeof(struct element*));
    for(j=0; j<n_nodes; j++)
      node->results[j] = NULL;
  }

  return  network;
}


void open_channel(struct network* network, gsl_rng* random_generator){
  long channel_id, edge1_id, edge2_id, node1_id, node2_id;
  channel_id = array_len(network->channels);
  edge1_id = array_len(network->edges);
  edge2_id = array_len(network->edges) + 1;
  node1_id = gsl_rng_uniform_int(random_generator, array_len(network->nodes));
  do{
    node2_id = gsl_rng_uniform_int(random_generator, array_len(network->nodes));
  } while(node2_id==node1_id);

  generate_random_channel(channel_id, edge1_id, edge2_id, node1_id, node2_id, 1000, network, random_generator);
}


//FUNCTIONS COPIED FROM ONE MILLION CHANNELS PROJECT: NOT WORKING, TOO BUGGY

//parameters of the power law to define number of channels per node, taken from millions-channel-project
/* #define A 2.114 */
/* #define B 3.887 */
/* #define C 5.837 */

/* double pow_law_integral(double x){ */
/*   double y; */
/*   y = (C*pow(x+B, (-1 * A)+1) / ((-1 * A)+1)); */
/*   return y; */
/* } */

/* double inverse_pow_law_integral(double y){ */
/*   double lower, upper, x; */
/*   lower = (((y) * ((-1* A)+1))/C); */
/*   upper = (1/((-1*A)+1)); */
/*   x = pow(lower, upper) - B; */
/*   return x; */
/* } */

/* double rand_to_power_law(int min, int max, gsl_rng* random_generator){ */
/*   double lower, upper, r, x; */
/*   lower = pow_law_integral(min); */
/*   upper = pow_law_integral(max); */
/*   r = lower + gsl_rng_uniform(random_generator)*(upper-lower); //formula to convert random in [0,1] to random in [lower, upper] */
/*   x = inverse_pow_law_integral(r); */
/*   return x; */
/* } */

/* /\* int compare_channels_per_node(const void* a, const void* b){ *\/ */
/* /\*   struct channels_per_node *node_a, *node_b; *\/ */

/* /\*   node_a = (struct channels_per_node*) a; *\/ */
/* /\*   node_b = (struct channels_per_node*) b; *\/ */

/* /\*   if(node_a->n_channels == node_b->n_channels) *\/ */
/* /\*     return 0; *\/ */
/* /\*   else if(node_a->n_channels < node_b->n_channels) *\/ */
/* /\*     return 1; *\/ */
/* /\*   else *\/ */
/* /\*     return -1; *\/ */
/* /\* } *\/ */

/* struct random_network_node* new_random_network_node(long id, int n_channels){ */
/*   struct random_network_node* node; */
/*   node = malloc(sizeof(struct random_network_node)); */
/*   node->id = id; */
/*   node->n_channels = n_channels; */
/*   node->channel_count = 0; */
/*   node->left = 1; */
/*   node->in_network = 0; */
/*   node->connections = NULL; */
/*   return node; */
/* } */

/* void insert_in_order(struct random_network_node **nodes, long inserted_nodes, struct random_network_node* new_node) { */
/*   long i, index; */

/*   for(i=0; i<inserted_nodes; i++){ */
/*     if(new_node->n_channels > nodes[i]->n_channels) */
/*       break; */
/*   } */
/*   index = i; */

/*   for(i=inserted_nodes; i > index; i-- ) */
/*     nodes[i] = nodes[i-1]; */

/*   nodes[index] = new_node; */
/* } */


/* long count_nodes_left(struct random_network_node **nodes, long n_nodes){ */
/*   long count=0, i; */
/*   for(i=0; i<n_nodes; i++){ */
/*     if(nodes[i]->left) */
/*       ++count; */
/*   } */
/*   return count; */
/* } */

/* int is_equal(long* a, long* b) { */
/*   return *a==*b; */
/* } */

/* unsigned int is_full(struct random_network_node* node){ */
/*   return (node->channel_count>=node->n_channels); */
/* } */

/* void initialize_random_network(struct network_params net_params, gsl_rng *random_generator) { */
/*   long node_id_counter, node_id, i, n_nodes_left, r, l, j, k; */
/*   int n_channels, channels_to_create; */
/*   double x; */
/*   FILE* nodes_file, *channels_file, *edges_file; */
/*   unsigned int done, node_done, full, equal, used, disconn; */
/*   struct random_network_node **nodes, *node, *node_to_connect, *n; */
/*   struct array* nodes_left; */

/*   nodes_file = fopen("nodes.csv", "w"); */
/*   if(nodes_file==NULL) { */
/*     printf("ERROR cannot open file nodes.csv\n"); */
/*     exit(-1); */
/*   } */
/*   fprintf(nodes_file, "id\n"); */

/*   channels_file = fopen("channels.csv", "w"); */
/*   if(channels_file==NULL) { */
/*     printf("ERROR cannot open file channels.csv\n"); */
/*     exit(-1); */
/*   } */
/*   fprintf(channels_file, "id,direction1,direction2,node1,node2,capacity,latency\n"); */

/*   edges_file = fopen("edges.csv", "w"); */
/*   if(edges_file==NULL) { */
/*     printf("ERROR cannot open file edges.csv\n"); */
/*     exit(-1); */
/*   } */
/*   fprintf(edges_file, "id,channel,other_direction,counterparty,balance,fee_base,fee_proportional,min_htlc,timelock\n"); */

/*   nodes = malloc(net_params.n_nodes*sizeof(struct graph_node* )); */
/*   nodes_left = array_initialize(net_params.n_nodes); */

/*   for (node_id_counter=0; node_id_counter<net_params.n_nodes; node_id_counter++){ */

/*     do{ */
/*       x = rand_to_power_law(0, net_params.n_channels, random_generator); */
/*       n_channels = round(x); */
/*     } while(n_channels <= 0 || n_channels > net_params.n_channels); */

/*     node = new_random_network_node(node_id_counter, n_channels); */
/*     if(node_id_counter==0){ */
/*       nodes[node_id_counter] = node; */
/*     } */
/*     else{ */
/*       insert_in_order(nodes, node_id_counter, node); */
/*     } */
/*     nodes_left = array_insert(nodes_left, &(node->id)); */

/*     fprintf(nodes_file, "%ld\n",node_id_counter); */
/*   } */

/*   done = 0; */
/*   nodes[0]->in_network = 1; */
/*   for(l=0; l<net_params.n_nodes; l++){ */
/*     node = nodes[l]; */
/*     node_done=0; */
/*     if(is_full(node)) { */
/*       channels_to_create = node->n_channels - node->channel_count; */
/*       for(i=0; i<channels_to_create; i++){ */
/*         n_nodes_left = count_nodes_left(nodes, net_params.n_nodes); */
/*         if(n_nodes_left<=1) { */
/*           done=1; */
/*           break; */
/*         } */
/*         if(list_len(node->connections) == net_params.n_nodes-1) */
/*           break; */
/*         r = gsl_rng_uniform_int(random_generator, array_len(nodes_left)); */
/*         node_to_connect = array_get(nodes_left, r); */
/*         full = is_full(node_to_connect); */
/*         equal = (node->id==node_to_connect->id); */
/*         used = is_in_list(node_to_connect->connections, node->id); */
/*         disconn=0; */
/*         if(i == channels_to_create-1){ */
/*           if(!(node->in_network) && !(node_to_connect->in_network)) */
/*             disconn=1; */
/*         } */
/*         j=0; */
/*         while((disconn || full || equal || used) && !node_done && !done){ */
/*           disconn = 0; */
/*           if(full) */
/*             array_delete(nodes_left, &(node_to_connect->id), is_equal); */
/*           else if(equal) */
/*             array_delete(nodes_left, &(node_to_connect->id), is_equal); */
/*           if(used) */
/*             ++j; */
/*           if(array_len(nodes_left)<=1) */
/*             done=1; */
/*           else if(j==5){ */
/*             k=0; */
/*             while(k<array_len(nodes_left)){ */
/*               node_done = 1; */
/*               n = array_get(nodes_left, k); */
/*               full = is_full(node_to_connect); */
/*               if() */

/*             } */
/*           } */


/*         } */






/*       } */

/*     } */
/*     //rimuovi il nodo left */
/*   } */


/*   exit(-1); */



/* } */
