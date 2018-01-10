LIBS=/home/Dottorato/GitHub/ScalingtheBlockchainSim/gc/lib/libgc.a -lgsl -lgslcblas -lm
INCLUDES=-I/home/Dottorato/GitHub/ScalingtheBlockchainSim/gc/include

build:  main.o ./utils/heap.o ./utils/hashTable.o ./utils/array.o ./simulator/event.o ./simulator/initialize.o ./protocol/protocol.o ./protocol/findRoute.o ./simulator/stats.o
	gcc -o sim.out main.o ./utils/heap.o ./utils/hashTable.o ./utils/array.o ./simulator/event.o ./simulator/initialize.o ./protocol/protocol.o ./protocol/findRoute.o ./simulator/stats.o  $(INCLUDES) $(LIBS)
run:
	GSL_RNG_SEED=1987  ./sim.out
clear:
	rm -r ./*.o
