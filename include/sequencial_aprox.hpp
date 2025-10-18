#ifndef JOHNSON_CYCLES_APROX_HPP
#define JOHNSON_CYCLES_APROX_HPP

#include "../common/graph.h"
#include <vector>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include "common/CycleTimer.h"

// Algoritmo de Johnson para contar todos os ciclos simples em um grafo dirigido
int johnson_cycles_approx(Graph G, int max_len);

#endif // JOHNSON_CYCLES_APROX_HPP