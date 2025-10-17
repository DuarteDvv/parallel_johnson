#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string>
#include <getopt.h>

#include <iostream>
#include <sstream>
#include <vector>

#include "common/CycleTimer.h"
#include "common/graph.h"
#include "parallel_v0.hpp"
#include "parallel_v1.hpp"
#include "sequencial.hpp"


#define USE_BINARY_GRAPH 1 // Load graph from binary file if available
#define DEBUG 0 // Print ?


int main(int argc, char** argv) {

    int  num_threads = -1;
    std::string graph_filename;

    if (argc < 2)
    {
        std::cerr << "Usage: <path/to/graph/file> [num_threads]\n";
        std::cerr << "  To run results for all thread counts: <path/to/graph/file>\n";
        std::cerr << "  Run with a certain number of threads (no correctness run): <path/to/graph/file> <num_threads>\n";
        exit(1);
    }

    int thread_count = -1;
    if (argc == 3)
    {
        thread_count = atoi(argv[2]);
    }

    graph_filename = argv[1];

    Graph g;

    printf("----------------------------------------------------------\n");
    printf("Max system threads = %d\n", omp_get_max_threads());
    if (thread_count > 0)
    {
        thread_count = std::min(thread_count, omp_get_max_threads());
        printf("Running with %d threads\n", thread_count);
    }
    printf("----------------------------------------------------------\n");

    printf("Loading graph...\n");
    if (USE_BINARY_GRAPH) {
      g = load_graph_binary(graph_filename.c_str());
    } else {
        g = load_graph(argv[1]);
        printf("storing binary form of graph!\n");
        store_graph_binary(graph_filename.append(".bin").c_str(), g);
        delete g;
        exit(1);
    }

    printf("\n");
    printf("Graph stats:\n");
    printf("  Edges: %d\n", g->num_edges);
    printf("  Nodes: %d\n", g->num_nodes);
    printf("\n");
    printf("----------------------------------------------------------\n");

    if (thread_count > 0)
        omp_set_num_threads(thread_count);
    else
        omp_set_num_threads(omp_get_max_threads());


    double start = CycleTimer::currentSeconds();
    int sol = johnson_cycles(g);
    double end = CycleTimer::currentSeconds();
    printf("Sequencial Johnson\n       Time taken: %.6f seconds\n", end - start);
    printf("       Number of simple cycles found: %d\n", sol);
    printf("----------------------------------------------------------\n");

    double start0 = CycleTimer::currentSeconds();
    int sol0 = johnson_cycles_parallel_v0(g);
    double end0 = CycleTimer::currentSeconds();
    printf("Parallel v0 Johnson (For main loop)\n       Time taken: %.6f seconds\n", end0 - start0);
    printf("       Number of simple cycles found: %d\n", sol0);
    printf("       Speedup: %.2f\n", (end - start) / (end0 - start0));
    printf("----------------------------------------------------------\n");

    double start1 = CycleTimer::currentSeconds();
    int sol1 = johnson_cycles_parallel_v1(g);
    double end1 = CycleTimer::currentSeconds();
    printf("Parallel v1 Johnson (Tasks)\n       Time taken: %.6f seconds\n", end1 - start1);
    printf("       Number of simple cycles found: %d\n", sol1);
    printf("       Speedup: %.2f\n", (end - start) / (end1 - start1));
    printf("----------------------------------------------------------\n");



    
   
    
    delete g;
    return 0;
}
