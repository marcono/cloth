#include makefile.variable

LIBS=-lgsl -lgslcblas -lm 
#INCLUDES=-I$(ipath)include/json-c -I$(ipath)include/gsl -I$(ipath)include/

build:
	gcc -g -pthread -o cloth ./src/cloth.c ./src/heap.c ./src/array.c ./src/list.c ./src/event.c ./src/payments.c ./src/htlc.c ./src/routing.c ./src/network.c ./src/utils.c $(LIBS)
run:
	GSL_RNG_SEED=1992  ./cloth
clear:
	rm -r ./*.o
