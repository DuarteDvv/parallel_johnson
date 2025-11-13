#ifndef PARALLEL_V5_HPP
#define PARALLEL_V5_HPP

#include "graph.h"
#include <vector>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include "CycleTimer.h"
#include <omp.h>
#include <atomic>
#include <mutex>

int johnson_cycles_parallel_v5(Graph G);

#endif // PARALLEL_V5_HPP
