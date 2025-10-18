#include "parallel_v2.hpp"

#define PARALLEL_DEPTH 4
#define PARALLEL_BRANCH 2
#define DEBUG 0

std::vector<std::vector<int>> BFS_foward_backward_SCCs_v2(Graph G, const std::vector<int>& active, int min_vertex){

    std::vector<std::vector<int>> SCCs;
    std::vector<int> removed(G->num_nodes, 0);
    for (int i = 0; i < G->num_nodes; i++) {
        if (active[i] == 0 || i < min_vertex) {
            removed[i] = 1;
        }
    }

    for (int pivot = min_vertex; pivot < G->num_nodes; pivot++) {
        if (removed[pivot] == 1) continue;

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
            frontier_fwd = std::move(new_frontier);
        }

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
            frontier_bwd = std::move(new_frontier);
        }

        std::vector<int> current_SCC;
        for (int i = 0; i < G->num_nodes; i++) {
            if (visited_fwd[i] && visited_bwd[i]) {
                current_SCC.push_back(i);
                removed[i] = 1;
            }
        }

        if (!current_SCC.empty()) {
            SCCs.push_back(std::move(current_SCC));
        }
    }

    return SCCs;
}

void unblock_v2(int u, std::vector<bool>& blocked, std::vector<std::unordered_set<int>>& B) {

    blocked[u] = false;
    for (int w : B[u]) {
        if (blocked[w]) {
            unblock_v2(w, blocked, B);
        }
    }
    B[u].clear();
}


bool circuit_v2_parallel(
    int v, 
    int s, 
    Graph G, 
    const std::unordered_set<int>& scc_set,
    std::vector<bool>& blocked, 
    std::vector<std::unordered_set<int>>& B,
    int& cycle_count, int depth = 0
) {

    bool found_cycle = false;
    blocked[v] = true;

    std::vector<int> neighbors;
    const Vertex* out_begin = outgoing_begin(G, v);
    const Vertex* out_end   = outgoing_end(G, v);
    neighbors.reserve(out_end - out_begin); // evita realocações
    for (const Vertex* neighbor = out_begin; neighbor != out_end; ++neighbor) {
        int w = *neighbor;
        if (w < s) continue;
        if (scc_set.find(w) == scc_set.end()) continue;
        neighbors.push_back(w);
    }

    const int branching = static_cast<int>(neighbors.size());
    const bool allow_spawn = (depth < PARALLEL_DEPTH) && (branching >= PARALLEL_BRANCH);

    std::atomic<bool> any_child_found(false);

    #pragma omp taskgroup
    {
        for (int i = 0; i < branching; ++i) {
            int w = neighbors[i];

            if (w == s) {
                #pragma omp atomic
                cycle_count++;
                found_cycle = true; // local ao pai (ok)

            } else if (!blocked[w]) {
                if (allow_spawn) {

                    std::vector<bool> blocked_copy = blocked;
                    std::vector<std::unordered_set<int>> B_copy = B;

                    #pragma omp task firstprivate(w, blocked_copy, B_copy, depth) shared(any_child_found, cycle_count, G, scc_set)
                    {
                        bool child_res = circuit_v2_parallel(w, s, G, scc_set, blocked_copy, B_copy,cycle_count, depth + 1);
                        if (child_res) any_child_found.store(true, std::memory_order_relaxed);
                    }
                } else {
                    if (circuit_v2_parallel(w, s, G, scc_set, blocked, B, cycle_count, depth + 1)) {
                        found_cycle = true;
                    }
                }
            }
        }
       
    }

    if (any_child_found.load(std::memory_order_relaxed)) {
        found_cycle = true;
    }

    if (found_cycle) {
        unblock_v2(v, blocked, B); 
    } else {
      
        for (int w : neighbors) {
            B[w].insert(v);
        }
    }

    return found_cycle;
}



int johnson_cycles_parallel_v2(Graph G) {
    int n = G->num_nodes;
    int s = 0;
    int cycle_count = 0;
    std::vector<int> active(G->num_nodes, 1); // todos ativos inicialmente

    while (s < n) {
        std::vector<std::vector<int>> SCCs = BFS_foward_backward_SCCs_v2(G, active, s);

        std::vector<int> scc_vertices;
        for (const std::vector<int>& scc : SCCs) {
            if (std::find(scc.begin(), scc.end(), s) != scc.end()) {
                scc_vertices = scc;
                break;
            }
        }

        if (scc_vertices.empty()) {
            active[s] = 0;
            s++;
            continue;
        }

        std::unordered_set<int> scc_set(scc_vertices.begin(), scc_vertices.end());

        std::vector<bool> blocked(n, false);
        std::vector<std::unordered_set<int>> B(n);
        std::vector<int> stack;

     
        #pragma omp parallel
        {
            #pragma omp single
            {
                circuit_v2_parallel(s, s, G, scc_set, blocked, B, cycle_count, 0);
            }
        
        }

        active[s] = 0;
        ++s;
    }

    return cycle_count;
}
