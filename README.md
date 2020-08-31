# CLoTH
CLoTH is a payment-channel network (PCN) simulator. Currently, it simulates the
Lightning Network, the mainstream payment-channel network, which is built on top of the
Bitcoin blockchain. 

The key feature and novelty of CLoTH is that it exactly reproduces the code of
the Lightning Network (specifically, the functions implementing routing and
HTLC mechanics). This ensures the validity of the simulation results produced by
CLoTH.

CLoTH is currently based on `lnd-v0.9.1-beta`, the Golang implementation of the
Lightning Network.

## Install libraries

Before building CLoTH, install the `gsl` library: 

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

CLoTH simulates the execution of payments in a network whose topology
reproduces the Lightning Network real one (using the [scale-free network
model](https://en.wikipedia.org/wiki/Scale-free_network]). See `cloth_input.txt`
for the complete list of input parameters.

At the end of the simulation, payment-related statistics (such as probability of
payment success, average payment time) are computed and stored in
`cloth_output.json`.

## References

For more details on (an older version of) CLoTH and initial simulation results,
please refer to my doctoral thesis:

CONOSCENTI, Marco. Capabilities and Limitations of Payment Channel Networks for
Blockchain Scalability. 2019. PhD Thesis. Politecnico di Torino. Available
[here](https://iris.polito.it/retrieve/handle/11583/2764132/283298/phd-thesis-marco-conoscenti-final.pdf).
