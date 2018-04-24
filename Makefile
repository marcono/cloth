include makefile.variable

LIBS= -L$(ipath)lib -lgsl -lgslcblas -lm -ljson-c -lpthread
INCLUDES=-I$(ipath)include/json-c -I$(ipath)include/gsl -I$(ipath)include/

build:
	gcc $(INCLUDES) -g -pthread -o sim.out main.c ./utils/heap.c ./utils/hashTable.c ./utils/array.c ./utils/list.c ./simulator/event.c ./simulator/initialize.c ./protocol/protocol.c ./protocol/findRoute.c ./simulator/stats.c  $(LIBS) 
run:
	LD_LIBRARY_PATH=$(ipath)lib GSL_RNG_SEED=1992  ./sim.out 
clear:
	rm -r ./*.o
