#include "parallel_v5.hpp"

#define PARALLEL_MIN_BRANCH 4
#define SCC_SEQUENTIAL_RATIO 0.8

std::vector<std::vector<int>> BFS_foward_backward_SCCs_v5(Graph G, const std::vector<int>& active, int min_vertex){

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

void unblock_v5(int u, std::vector<bool>& blocked, std::vector<std::vector<int>>& B) {

    blocked[u] = false;
    for (int w : B[u]) {
        if (blocked[w]) {
            unblock_v5(w, blocked, B);
        }
    }
    B[u].clear();
}

bool circuit_v5_sequential(int v, int s, Graph G, const std::vector<char>& scc_mask,
            std::vector<bool>& blocked, std::vector<std::vector<int>>& B,
            int& cycle_count) {

    bool found_cycle = false;
    blocked[v] = true;

    const Vertex* out_begin = outgoing_begin(G, v);
    const Vertex* out_end = outgoing_end(G, v);

    for (const Vertex* neighbor = out_begin; neighbor != out_end; ++neighbor) {
        int w = *neighbor;
        if (w < s) continue;
        if (!scc_mask[w]) continue;

        if (w == s) {
            ++cycle_count;
            found_cycle = true;
        } else if (!blocked[w]) {
            if (circuit_v5_sequential(w, s, G, scc_mask, blocked, B, cycle_count)) {
                found_cycle = true;
            }
        }
    }

    if (found_cycle) {
        unblock_v5(v, blocked, B);
    } else {
        for (const Vertex* neighbor = out_begin; neighbor != out_end; ++neighbor) {
            int w = *neighbor;
            if (w < s) continue;
            if (!scc_mask[w]) continue;
            auto& bucket = B[w];
            if (std::find(bucket.begin(), bucket.end(), v) == bucket.end()) {
                bucket.push_back(v);
            }
        }
    }

    return found_cycle;
}



bool circuit_v5_parallel(
    int v, 
    int s, 
    Graph G, 
    const std::vector<char>& scc_mask,
    std::vector<bool>& blocked, 
    std::vector<std::vector<int>>& B,
    int& cycle_count, 
    int depth = 0
) {

    bool found_cycle = false;
    blocked[v] = true;


    std::vector<int> neighbors;
    const Vertex* out_begin = outgoing_begin(G, v);
    const Vertex* out_end   = outgoing_end(G, v);
    neighbors.reserve(out_end - out_begin);

    for (const Vertex* neighbor = out_begin; neighbor != out_end; ++neighbor) {
        int w = *neighbor;
        if (w < s) continue;
        if (!scc_mask[w]) continue;
        neighbors.push_back(w);
    }

    const int branching = static_cast<int>(neighbors.size());

    // Permitir spawn apenas no nível topo (depth==0) e quando o branching
    // é suficiente para amortizar o overhead de tarefas/copias.
    const bool allow_spawn =
        (depth == 0) &&
        (branching >= PARALLEL_MIN_BRANCH);

    std::atomic<bool> any_child_found(false);

    int local_cycles_parent = 0;

    #pragma omp taskgroup
    {
        for (int i = 0; i < branching; ++i) {
            int w = neighbors[i];

            if (w == s) {
                local_cycles_parent++;
                found_cycle = true;
            } else if (!blocked[w]) {
                if (allow_spawn) {
                    // Cada tarefa usa seu próprio estado para preservar o pruning do Johnson
                    std::vector<bool> blocked_copy = blocked;
                    std::vector<std::vector<int>> B_copy = B;

                    #pragma omp task firstprivate(w, blocked_copy, B_copy, depth) shared(any_child_found, cycle_count, G, scc_mask)
                    {
                        int local_count = 0;
                        bool child_res = circuit_v5_sequential(w, s, G, scc_mask, blocked_copy, B_copy, local_count);
                        if (local_count > 0) {
                            #pragma omp atomic
                            cycle_count += local_count;
                        }
                        if (child_res) any_child_found.store(true, std::memory_order_relaxed);
                    }
                } else {

                    if (circuit_v5_parallel(w, s, G, scc_mask, blocked, B, cycle_count, depth + 1)) {
                        found_cycle = true;
                    }
                }
            }
        }
        
    }

    if (local_cycles_parent > 0) {
        #pragma omp atomic
        cycle_count += local_cycles_parent;
    }

    if (any_child_found.load(std::memory_order_relaxed)) {
        found_cycle = true;
    }

    if (found_cycle) {
        unblock_v5(v, blocked, B);

    } else {
        
        for (int w : neighbors) {
            auto& bucket = B[w];
            if (std::find(bucket.begin(), bucket.end(), v) == bucket.end()) {
                bucket.push_back(v);
            }
        }
    }


    return found_cycle;
}


// Estrutura para representar um trabalho (subgrafo SCC) a ser processado
struct SCCJob {
    int s;                           // vértice inicial
    std::vector<int> scc_vertices;   // vértices da SCC
    std::vector<char> scc_mask;      // máscara da SCC
};


int johnson_cycles_parallel_v5(Graph G) {
    int n = G->num_nodes;
    std::atomic<int> cycle_count(0);
    std::vector<int> active(G->num_nodes, 1); // todos ativos inicialmente
    
    // Mutex para proteger a lista de active e o índice s
    std::mutex active_mutex;
    
    #pragma omp parallel
    {
        #pragma omp single
        {
            int s = 0;
            while (s < n) {
                // Encontra SCCs de forma sequencial (mas permite tasks de SCCs anteriores rodarem)
                std::vector<std::vector<int>> SCCs = BFS_foward_backward_SCCs_v5(G, active, s);

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

                // Cria a máscara da SCC
                std::vector<char> scc_mask(n, 0);
                for (int v : scc_vertices) {
                    scc_mask[v] = 1;
                }

                const int current_s = s;
                const bool use_sequential = s > n * SCC_SEQUENTIAL_RATIO;

                // Cria uma task para processar esta SCC
                // Isso permite que múltiplas SCCs sejam processadas em paralelo
                #pragma omp task firstprivate(current_s, scc_mask, use_sequential) shared(G, cycle_count)
                {
                    std::vector<bool> blocked(n, false);
                    std::vector<std::vector<int>> B(n);

                    int local_cycle_count = 0;

                    if (use_sequential) {
                        circuit_v5_sequential(current_s, current_s, G, scc_mask, blocked, B, local_cycle_count);
                    } else {
                        // Cria uma região paralela aninhada para explorar os ramos da SCC
                        #pragma omp taskgroup
                        {
                            circuit_v5_parallel(current_s, current_s, G, scc_mask, blocked, B, local_cycle_count, 0);
                        }
                    }

                    if (local_cycle_count > 0) {
                        cycle_count.fetch_add(local_cycle_count, std::memory_order_relaxed);
                    }
                }

                active[s] = 0;
                ++s;
            }
        }
    }

    return cycle_count.load();
}
