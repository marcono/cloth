include makefile.variable

LIBS= -L$(ipath)lib -lgsl -lgslcblas -lm -ljson-c -lpthread
INCLUDES=-I$(ipath)include/json-c -I$(ipath)include/gsl -I$(ipath)include/

build:
	gcc $(INCLUDES) -g -pthread -o cloth ./src/cloth.c ./src/heap.c ./src/array.c ./src/list.c ./src/event.c ./src/payments.c ./src/htlc.c ./src/routing.c ./src/network.c $(LIBS)
run:
	LD_LIBRARY_PATH=$(ipath)lib GSL_RNG_SEED=1992  ./cloth 
clear:
	rm -r ./*.o
