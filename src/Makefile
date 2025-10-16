all: default

default: main.cpp
	g++ -I../ -std=c++11 -fopenmp -O3 -g -o sccs main.cpp ../common/graph.cpp kosaraju.cpp johnson_cycles.cpp
clean:
	rm -rf sccs  *~ *.*~