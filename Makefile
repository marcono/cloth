LIBS=/home/Dottorato/GitHub/ScalingtheBlockchainSim/gc/lib/libgc.a -lgsl -lgslcblas -lm
INCLUDES=-I/home/Dottorato/GitHub/ScalingtheBlockchainSim/gc/include

build:  main.o ./utils/heap.o ./utils/hashTable.o ./simulator/event.o ./simulator/initialize.o ./protocol/protocol.o
	gcc -o sim.out main.o ./utils/heap.o ./utils/hashTable.o ./simulator/event.o ./simulator/initialize.o ./protocol/protocol.o $(INCLUDES) $(LIBS)
run:
	GSL_RNG_SEED=1992  ./sim.out
clear:
	rm ./*.o
