#ifndef PARALLEL_V3_HPP
#define PARALLEL_V3_HPP

#include "graph.h"
#include <vector>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include "CycleTimer.h"
#include <omp.h>
#include <atomic>

int johnson_cycles_parallel_v3(Graph G);

#endif // PARALLEL_V3_HPP

