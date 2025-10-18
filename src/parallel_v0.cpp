
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <omp.h>
#include "common/CycleTimer.h"
#include "parallel_v0.hpp"

#define DEBUG 0

std::vector<std::vector<int>> BFS_foward_backward_SCCs_v0(Graph G, const std::vector<int>& active, int min_vertex){
    
    std::vector<std::vector<int>> SCCs;
    std::vector<int> removed(G->num_nodes, 0);
    
    for (int i = 0; i < G->num_nodes; i++) {
        if (active[i] == 0 || i < min_vertex) {
            removed[i] = 1;
        }
    }

    for (int pivot = min_vertex; pivot < G->num_nodes; pivot++) {

        if (removed[pivot] == 1) continue; 

        // Forward BFS
        std::vector<bool> visited_fwd(G->num_nodes, false);
        std::vector<int> frontier_fwd;
        frontier_fwd.push_back(pivot);
        visited_fwd[pivot] = true;

        while (!frontier_fwd.empty()) {
            std::vector<int> new_frontier;
            for (int u : frontier_fwd) {
                const Vertex* out_begin = outgoing_begin(G, u);
                const Vertex* out_end = outgoing_end(G, u);
                for (const Vertex* neighbor = out_begin; neighbor != out_end; ++neighbor) {
                    int v = *neighbor;
                    if (removed[v] == 0 && !visited_fwd[v]) {
                        visited_fwd[v] = true;
                        new_frontier.push_back(v);
                    }
                }
            }
            frontier_fwd = new_frontier;
        }

        // Backward BFS
        std::vector<bool> visited_bwd(G->num_nodes, false);
        std::vector<int> frontier_bwd;
        frontier_bwd.push_back(pivot);
        visited_bwd[pivot] = true;

        while (!frontier_bwd.empty()) {
            std::vector<int> new_frontier;
            for (int u : frontier_bwd) {
                const Vertex* in_begin = incoming_begin(G, u);
                const Vertex* in_end = incoming_end(G, u);
                for (const Vertex* neighbor = in_begin; neighbor != in_end; ++neighbor) {
                    int v = *neighbor;
                    if (removed[v] == 0 && !visited_bwd[v]) {
                        visited_bwd[v] = true;
                        new_frontier.push_back(v);
                    }
                }
            }
            frontier_bwd = new_frontier;
        }

        // Intersecção = vértices alcançáveis nos dois sentidos = SCC
        std::vector<int> current_SCC;
        for (int i = 0; i < G->num_nodes; i++) {
            if (visited_fwd[i] && visited_bwd[i]) {
                current_SCC.push_back(i);
                removed[i] = 1; 
            }
        }

        // Adiciona SCC encontrada (mesmo se tiver 1 vértice)
        if (!current_SCC.empty()) {
            SCCs.push_back(current_SCC);
        }
    }

    return SCCs;
}

void unblock_v0(int u, std::vector<bool>& blocked, std::vector<std::unordered_set<int>>& B) {
    blocked[u] = false;
    for (int w : B[u]) {
        if (blocked[w]) {
            unblock_v0(w, blocked, B);
        }
    }
    B[u].clear();
}


bool circuit_v0(int v, int s, Graph G, const std::unordered_set<int>& scc_set,
            std::vector<bool>& blocked, std::vector<std::unordered_set<int>>& B,
        int& cycle_count) {

    bool found_cycle = false;
    blocked[v] = true;

    const Vertex* out_begin = outgoing_begin(G, v);
    const Vertex* out_end = outgoing_end(G, v);

    for (const Vertex* neighbor = out_begin; neighbor != out_end; ++neighbor) {
        int w = *neighbor;
        // Ignora vértices fora da SCC ou menores que s
        if (scc_set.find(w) == scc_set.end() || w < s) {
            continue;
        }

        if (w == s) {
            // Ciclo encontrado
            cycle_count++;
            found_cycle = true;
        } else if (!blocked[w]) {
            if (circuit_v0(w, s, G, scc_set, blocked, B, cycle_count)) {
                found_cycle = true;
            }
        }
    }

    if (found_cycle) {
        unblock_v0(v, blocked, B);
    } else {
        for (const Vertex* neighbor = out_begin; neighbor != out_end; ++neighbor) {
            int w = *neighbor;
            // Ignora vértices fora da SCC ou menores que s
            if (scc_set.find(w) == scc_set.end() || w < s) {
                continue;
            }
            B[w].insert(v);
        }
    }

    return found_cycle;

}


int johnson_cycles_parallel_v0(Graph G) {
    int n = G->num_nodes;
    int cycle_count = 0;
    std::vector<int> active(G->num_nodes, 1);
    
    
    #pragma omp parallel
    {
        int local_cycle_count = 0;
        
        // paralelização do loop principal sobre s, cada thread processa valores diferentes de s
        #pragma omp for schedule(dynamic, 1)
        for (int s = 0; s < n; s++) {

            double start = CycleTimer::currentSeconds();
            
            // cópia local 
            std::vector<int> local_active = active;
            
            for (int i = 0; i < s; i++) {
                local_active[i] = 0;
            }
            
            std::vector<std::vector<int>> SCCs = BFS_foward_backward_SCCs_v0(G, local_active, s);
            
            std::vector<int> scc_vertices;
            for (const std::vector<int>& scc : SCCs) {
                if (std::find(scc.begin(), scc.end(), s) != scc.end()) {
                    scc_vertices = scc;
                    break;
                }
            }
            
            if (scc_vertices.empty()) {
                continue;
            }
            
            std::unordered_set<int> scc_set(scc_vertices.begin(), scc_vertices.end());
            std::vector<bool> blocked(n, false);
            std::vector<std::unordered_set<int>> B(n);
 
            
            circuit_v0(s, s, G, scc_set, blocked, B, local_cycle_count);

            double end = CycleTimer::currentSeconds();

            if (DEBUG) { //debug de gargalo
                printf("Thread %d processed s=%d in %.6f seconds, found %d cycles.\n",
                       omp_get_thread_num(), s, end - start, local_cycle_count);
            }
        }
        
        #pragma omp atomic
        cycle_count += local_cycle_count;
    }
    
    return cycle_count;
}