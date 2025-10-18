#ifndef PARALLEL_MPI_HPP
#define PARALLEL_MPI_HPP


#include "graph.h"
#include <vector>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include "CycleTimer.h"
#include <omp.h>
#include <atomic>

void johnson_cycles_parallel_mpi(Graph g);

#endif // PARALLEL_MPI_HPP