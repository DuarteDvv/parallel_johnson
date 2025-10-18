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
#include "parallel_v2.hpp"
#include "parallel_v3.hpp"
#include "parallel_v4.hpp"
#include "sequencial.hpp"



int main(int argc, char** argv) {

    // CLI options
    int version = -1;            // -1 => run all; 0 => sequential; 1..5 => v0..v4
    int use_binary_graph = 1;    // 1 => load binary graph; 0 => load from text

    std::string graph_filename;

    // Parse flags: -v <int>, -s
    // Remaining args: <path/to/graph/file> [num_threads]
    int opt;
    while ((opt = getopt(argc, argv, "v:s")) != -1) {
        switch (opt) {
            case 'v':
                version = atoi(optarg);
                break;
            case 's':
                use_binary_graph = 0;
                break;
            default:
                std::cerr << "Usage: [-v N] [-s] <path/to/graph/file> [num_threads]\n";
                return 1;
        }
    }

    if (optind >= argc) {
        std::cerr << "Usage: [-v N] [-s] <path/to/graph/file> [num_threads]\n";
        std::cerr << "  -v N : version to execute (0=sequential, 1=v0, 2=v1, 3=v2, 4=v3, 5=v4). If omitted, run all.\n";
        std::cerr << "  -s   : load graph from text (disable binary).\n";
        return 1;
    }

    int thread_count = -1;
    graph_filename = argv[optind++];
    if (optind < argc) {
        thread_count = atoi(argv[optind]);
    }

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
    if (use_binary_graph) {
        g = load_graph_binary(graph_filename.c_str());
    } else {
        g = load_graph(graph_filename.c_str());
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


    auto run_seq = [&](bool print_header) {
        double t0 = CycleTimer::currentSeconds();
        int sol = johnson_cycles(g);
        double t1 = CycleTimer::currentSeconds();
        if (print_header) printf("Sequencial Johnson\n       Time taken: %.6f seconds\n", t1 - t0);
        else printf("Time taken: %.6f seconds\n", t1 - t0);
        printf("       Number of simple cycles found: %d\n", sol);
        printf("----------------------------------------------------------\n");
        return t1 - t0;
    };

    double seq_time = -1.0; // baseline

    auto run_v0 = [&]() {
        double t0 = CycleTimer::currentSeconds();
        int sol = johnson_cycles_parallel_v0(g);
        double t1 = CycleTimer::currentSeconds();
        printf("Parallel v0 Johnson (For main loop)\n       Time taken: %.6f seconds\n", t1 - t0);
        printf("       Number of simple cycles found: %d\n", sol);
        if (seq_time > 0.0) printf("       Speedup: %.2f\n", seq_time / (t1 - t0));
        printf("----------------------------------------------------------\n");
    };
    auto run_v1 = [&]() {
        double t0 = CycleTimer::currentSeconds();
        int sol = johnson_cycles_parallel_v1(g);
        double t1 = CycleTimer::currentSeconds();
        printf("Parallel v1 Johnson (Tasks)\n       Time taken: %.6f seconds\n", t1 - t0);
        printf("       Number of simple cycles found: %d\n", sol);
        if (seq_time > 0.0) printf("       Speedup: %.2f\n", seq_time / (t1 - t0));
        printf("----------------------------------------------------------\n");
    };
    auto run_v2 = [&]() {
        double t0 = CycleTimer::currentSeconds();
        int sol = johnson_cycles_parallel_v2(g);
        double t1 = CycleTimer::currentSeconds();
        printf("Parallel v2 Johnson (Taskgroup)\n       Time taken: %.6f seconds\n", t1 - t0);
        printf("       Number of simple cycles found: %d\n", sol);
        if (seq_time > 0.0) printf("       Speedup: %.2f\n", seq_time / (t1 - t0));
        printf("----------------------------------------------------------\n");
    };
    auto run_v3 = [&]() {
        double t0 = CycleTimer::currentSeconds();
        int sol = johnson_cycles_parallel_v3(g);
        double t1 = CycleTimer::currentSeconds();
        printf("Parallel v3 Johnson (Hybrid)\n       Time taken: %.6f seconds\n", t1 - t0);
        printf("       Number of simple cycles found: %d\n", sol);
        if (seq_time > 0.0) printf("       Speedup: %.2f\n", seq_time / (t1 - t0));
        printf("----------------------------------------------------------\n");
    };
    auto run_v4 = [&]() {
        double t0 = CycleTimer::currentSeconds();
        int sol = johnson_cycles_parallel_v4(g);
        double t1 = CycleTimer::currentSeconds();
        printf("Parallel v4 Johnson (Window Spawn)\n       Time taken: %.6f seconds\n", t1 - t0);
        printf("       Number of simple cycles found: %d\n", sol);
        if (seq_time > 0.0) printf("       Speedup: %.2f\n", seq_time / (t1 - t0));
        printf("----------------------------------------------------------\n");
    };

    if (version < 0) {
        // Run all versions
        seq_time = run_seq(true);
        run_v0();
        run_v1();
        run_v2();
        run_v3();
        run_v4();
    } else {
        // Run a specific version
        switch (version) {
            case 0:
                seq_time = run_seq(true);
                break;
            case 1:
                seq_time = run_seq(true);
                run_v0();
                break;
            case 2:
                seq_time = run_seq(true);
                run_v1();
                break;
            case 3:
                seq_time = run_seq(true);
                run_v2();
                break;
            case 4:
                seq_time = run_seq(true);
                run_v3();
                break;
            case 5:
                seq_time = run_seq(true);
                run_v4();
                break;
            default:
                std::cerr << "Invalid -v value. Use 0 (seq), 1 (v0), 2 (v1), 3 (v2), 4 (v3), 5 (v4).\n";
                delete g;
                return 1;
        }
    }

    delete g;
    return 0;
}
