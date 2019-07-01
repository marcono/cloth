# CLoTH
Simulator of HTLC payment networks

## Pre-requisites
Install pre-requisites

```sh
sudo apt install curl
sudo apt install gcc
sudo apt install make
sudo apt install git
sudo apt install autoconf automake libtool
```
## Install libraries

Install the libraries used by the simulator ([json-c](https://github.com/json-c/json-c) and [gsl-2.4](https://mirror2.mirror.garr.it/mirrors/gnuftp/gsl/gsl-2.4.tar.gz) by running in the project directory

```sh
./install.sh <path>
```
where `path` is the absolute path where to download and install the libraries.

## Build

Build the simulator by typing in the project directory

```sh
make build
```

## Run

Run the simulator by typing in the project directory

```sh
./run.sh <seed> <is-preprocess> <path-results>
```
where `seed` is the seed of the random variables used in the simulator and `pre-process` is the flag (0 or 1) which specifies whether the simulator is run or not in pre-processing mode. 

If the simulator is in pre-processing mode, it reads pre-input parameters from file `preinput.json`, it generates a topology and payments to be simulated using the above-mentioned parameters and stores them in files `peer.csv`, `channel.csv`, `channelInfo.csv`, `payment.csv`. Then, it runs the simulation.

If the simulator is not in pre-processing mode, it reads the topology from `peerLN.csv`, `channelLN.csv`, `channelInfoLN.csv` (which contain the data on the current LN mainnet) and generates payments to be simulated using the parameters in `preinput.json`. Then, it runs the simulation.

At the end, the simulator produces output measures in the file `output.json`. This file and all the others produced by the simulator are stored in path `path-results`.

## References

[1] Conoscenti, M.; Vetrò, A.; De Martin, J.C.; Spini, F. The CLoTH Simulator for HTLC Payment Networks with Introductory Lightning Network Performance Results. Information 2018, 9, 223. [](https://www.mdpi.com/2078-2489/9/9/223)

