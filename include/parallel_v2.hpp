#ifndef PARALLEL_V2_HPP
#define PARALLEL_V2_HPP

#include "graph.h"
#include <vector>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include "CycleTimer.h"
#include <omp.h>
#include <atomic>

int johnson_cycles_parallel_v2(Graph G);

#endif // PARALLEL_V2_HPP

