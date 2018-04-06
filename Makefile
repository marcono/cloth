LIBS=/home/Dottorato/GitHub/ScalingtheBlockchainSim/gc/lib/libgc.a -lgsl -lgslcblas -lm -ljson-c -lpthread
INCLUDES=-I/home/Dottorato/GitHub/ScalingtheBlockchainSim/gc/include

build:
	gcc -g -pthread -o sim.out main.c ./utils/heap.c ./utils/hashTable.c ./utils/array.c ./utils/list.c ./simulator/event.c ./simulator/initialize.c ./protocol/protocol.c ./protocol/findRoute.c ./simulator/stats.c  $(INCLUDES) $(LIBS)
run:
	GSL_RNG_SEED=2006  ./sim.out 
clear:
	rm -r ./*.o
