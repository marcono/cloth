# CLoTH
CLoTH is a payment-channel network (PCN) simulator. Currently, it simulates the
Lightning Network, the mainstream payment-channel network, which is built on top of the
Bitcoin blockchain. 

The key feature and novelty of CLoTH is that it exactly reproduces the code of
the Lightning Network (specifically, the functions implementing routing and
HTLC mechanics). This ensures the validity of the simulation results produced by
CLoTH.

CLoTH simulates the execution of payments in a payment channel network. The
input parameters of the simulation are read from `cloth_input.txt` file (see
below for more details). CLoTH outputs payment-related statistics (such as
probability of payment success, average payment time) in `cloth_output.json`.

CLoTH is currently based on `lnd-v0.10.0-beta`, the Golang implementation of the
Lightning Network.

## Install libraries

Install the `gsl` library: 

```sh
sudo apt install gsl
```

## Build

Build CLoTH:

```sh
make build
```

## Run

Run CLoTH:

```sh
./run.sh
```

### Input Parameters

The simulation input parameters are read from `cloth_input.txt` and are the
following:
- `generate_network_from_file`. Possible values: `true` or `false`. It indicates
  whether the network of the simulation is generated randomly
  (`generate_network_from_file=false`) or it is taken from csv files
  (`generate_network_from_file=true`).
- `nodes_filename`, `channels_filename`, `edges_filename`. In case
  `generate_network_from_file=true`, the names of the csv files where nodes,
  channels and edges of the network are taken from. See the templates of these
  files in `nodes_template.csv`, `channels_template.csv`, `edges_template.csv`.
- `n_additional_nodes`. In case of randomly generated network, the number of
  nodes in addition to the ones of the network model. The network model is a
  snapshot of the Lightning Network (see files `nodes_ln.csv` and
  `channels_ln.csv`): the random network is generated from this snapshot using
  the [scale-free network model](https://en.wikipedia.org/wiki/Scale-free_network).
- `n_channels_per_node`. In case of randomly generated network, the number of
  channels per additional node.
- `capacity_per_channel`. In case of randomly generated network, the average
  capacity of payment channels in satoshis.
- `faulty_node_probability`. The probability (between 0 and 1) that a node is
  faulty when forwarding a payment.
- `generate_payments_from_file`. Possible values: `true` or `false`. It
  indicates whether the payments of the simulation are generated randomly
  (`generate_payments_from_file=false`) or they are taken from a csv file
  (`generate_network_from_file=true`).
- `payments_filename`. In case `generate_payments_from_file=true`, the names of
  the csv files where the payments of the simulation are taken from. See the
  templates of this file in `payments_template.csv`.
- `payment_rate`. In case of randomly generated payments, the number of payments
  per second.
- `n_payments`. In case of randomly generated payments, the total number of
  payments to be simulated.
- `average_payment_amount`. In case of randomly generated payments, the average
  payment amount in satoshis.


## References

For more details on (an older version of) CLoTH and initial simulation results,
please refer to my doctoral thesis:

CONOSCENTI, Marco. Capabilities and Limitations of Payment Channel Networks for
Blockchain Scalability. 2019. PhD Thesis. Politecnico di Torino. Available
[here](https://iris.polito.it/retrieve/handle/11583/2764132/283298/phd-thesis-marco-conoscenti-final.pdf).
