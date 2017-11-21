LIBS=/home/Dottorato/GitHub/ScalingtheBlockchainSim/gc/lib/libgc.a
INCLUDES=-I/home/Dottorato/GitHub/ScalingtheBlockchainSim/gc/include

build:  main.o ./utils/heap.o ./simulator/event.o
	gcc -o sim.out main.o ./utils/heap.o ./simulator/event.o $(INCLUDES) $(LIBS)
run:
	./sim.out
clear:
	rm ./*.o
