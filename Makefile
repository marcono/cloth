LIBS=-lgsl -lgslcblas -lm -ljson-c -lpthread

build:
	gcc -g -pthread -o sim.out main.c ./utils/heap.c ./utils/hashTable.c ./utils/array.c ./utils/list.c ./simulator/event.c ./simulator/initialize.c ./protocol/protocol.c ./protocol/findRoute.c ./simulator/stats.c   $(LIBS)
run:
	GSL_RNG_SEED=1992  ./sim.out 
clear:
	rm -r ./*.o
