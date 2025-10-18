#include "sequencial.hpp"
#include <unordered_set>
#include <algorithm>
#include <vector>
#include <cstdio>

#define DEBUG 0

// Reuso da decomposição em SCCs da versão sequencial
extern std::vector<std::vector<int>> BFS_foward_backward_SCCs(
    Graph G, const std::vector<int>& active, int min_vertex);

// Função local para evitar colisão de símbolos com a versão sequencial
static void unblock_aprox(
    int u,
    std::vector<bool>& blocked,
    std::vector<std::unordered_set<int>>& B
) {
    blocked[u] = false;
    for (int w : B[u]) {
        if (blocked[w]) {
            unblock_aprox(w, blocked, B);
        }
    }
    B[u].clear();
}

// DFS limitada: só considera ciclos com tamanho <= max_len
static bool circuit_aprox(
    int v,
    int s,
    Graph G,
    const std::unordered_set<int>& scc_set,
    std::vector<bool>& blocked,
    std::vector<std::unordered_set<int>>& B,
    int& cycle_count,
    int max_len,
    int curr_len
) {
    bool found_cycle = false;
    blocked[v] = true;

    const Vertex* out_begin = outgoing_begin(G, v);
    const Vertex* out_end   = outgoing_end(G, v);

    for (const Vertex* neighbor = out_begin; neighbor != out_end; ++neighbor) {
        int w = *neighbor;

        if (w < s) continue;
        if (scc_set.find(w) == scc_set.end()) continue;

        int next_len = curr_len + 1;

        if (w == s) {
            if (next_len <= max_len) {
                ++cycle_count;
                found_cycle = true;
            }
        } else if (!blocked[w]) {
            // Só desce se ainda há espaço para possivelmente fechar em s
            if (next_len < max_len) {
                if (circuit_aprox(w, s, G, scc_set, blocked, B, cycle_count, max_len, next_len)) {
                    found_cycle = true;
                }
            }
        }
    }

    if (found_cycle) {
        unblock_aprox(v, blocked, B);
    } else {
        for (const Vertex* neighbor = out_begin; neighbor != out_end; ++neighbor) {
            int w = *neighbor;
            if (w < s) continue;
            if (scc_set.find(w) == scc_set.end()) continue;
            B[w].insert(v);
        }
    }

    return found_cycle;
}

// API pública: conta ciclos com comprimento <= max_len
int johnson_cycles_approx(Graph G, int max_len)
{
    int n = G->num_nodes;
    int s = 0;
    int cycle_count = 0;
    std::vector<int> active(n, 1);

    double SCC_time = 0.0;
    double circuit_time = 0.0;

    while (s < n) {
        double startSCC = CycleTimer::currentSeconds();
        std::vector<std::vector<int>> SCCs = BFS_foward_backward_SCCs(G, active, s);
        double endSCC = CycleTimer::currentSeconds();
        SCC_time += (endSCC - startSCC);

        std::vector<int> scc_vertices;
        for (const std::vector<int>& scc : SCCs) {
            if (std::find(scc.begin(), scc.end(), s) != scc.end()) {
                scc_vertices = scc;
                break;
            }
        }

        if (scc_vertices.empty()) {
            active[s] = 0;
            ++s;
            continue;
        }

        std::unordered_set<int> scc_set(scc_vertices.begin(), scc_vertices.end());
        std::vector<bool> blocked(n, false);
        std::vector<std::unordered_set<int>> B(n);

        double startCircuit = CycleTimer::currentSeconds();
        circuit_aprox(s, s, G, scc_set, blocked, B, cycle_count, max_len, 0);
        double endCircuit = CycleTimer::currentSeconds();
        circuit_time += (endCircuit - startCircuit);

        active[s] = 0;
        ++s;
    }

    if (DEBUG) {
        std::printf("Approx (<=%d) Total SCC time: %.6f seconds\n", max_len, SCC_time);
        std::printf("Approx (<=%d) Total circuit time: %.6f seconds\n", max_len, circuit_time);
    }

    return cycle_count;
}