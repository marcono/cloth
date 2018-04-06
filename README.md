# PaymentNetworkSim
Simulator of off-chain scaling solutions for the blockchain

## How to use

Run the simulator by typing `make run`.

The simulator asks whether you want to create random topology reading data from `preinput.json` . If so, you have to define the parameters in that file. Then the simulator creates the files `peer.csv`, `channel.csv`, `channelInfo.csv`, `payment.csv` and gives you the possibility to change the values in these files by displaying "Change topology and press enter". After pressing enter, the simulation will start using the data read from these files.

Instead, if you decide not to create a random topology, the simulator reads payments, channels and peers from `channelInfo_output.csv`, `channel_output.csv`, `payment_output.csv`, `peer_output.csv`. These files are created after each simulation run, and contains the values of peers, channels and payments after the simulation.

The performance measures computed during a simulation run are printed in the file `output.json`.
