#include "parallel_v1.hpp"


#define DEBUG 0

std::vector<std::vector<int>> BFS_foward_backward_SCCs_v1(Graph G, const std::vector<int>& active, int min_vertex){
    
    std::vector<std::vector<int>> SCCs;

    // removed[i] = 1 significa que já foi processado (já está em alguma SCC)
    std::vector<int> removed(G->num_nodes, 0);
    
    // Marca vértices inativos como já removidos, e também vértices < min_vertex
    for (int i = 0; i < G->num_nodes; i++) {
        if (active[i] == 0 || i < min_vertex) {
            removed[i] = 1;
        }
    }

    for (int pivot = min_vertex; pivot < G->num_nodes; pivot++) {

        if (removed[pivot] == 1) continue; // já removido ou inativo

        // BFS forward (seguindo arestas de saída)
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

        // BFS backward (seguindo arestas de entrada)
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
                removed[i] = 1; // marca como processado
            }
        }

        // Adiciona SCC encontrada (mesmo se tiver 1 vértice)
        if (!current_SCC.empty()) {
            SCCs.push_back(current_SCC);
        }
    }

    return SCCs;
}

void unblock_v1(int u, std::vector<bool>& blocked, std::vector<std::unordered_set<int>>& B) {
    blocked[u] = false;
    for (int w : B[u]) {
        if (blocked[w]) {
            unblock_v1(w, blocked, B);
        }
    }
    B[u].clear();
}


bool circuit_v1(int v, int s, Graph G, const std::unordered_set<int>& scc_set,
            std::vector<bool>& blocked, std::vector<std::unordered_set<int>>& B,
            std::vector<int>& stack, int& cycle_count) {

    bool found_cycle = false;
    stack.push_back(v);
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
            if (DEBUG) {
                std::cout << "Cycle " << cycle_count << ": ";
                for (int node : stack) {
                    std::cout << node << " ";
                }
                std::cout << s << std::endl;
            }
            found_cycle = true;
        } else if (!blocked[w]) {
            if (circuit_v1(w, s, G, scc_set, blocked, B, stack, cycle_count)) {
                found_cycle = true;
            }
        }
    }

    if (found_cycle) {
        unblock_v1(v, blocked, B);
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

    stack.pop_back();
    return found_cycle;

}

int johnson_cycles_parallel_v1(Graph G) {

    int n = G->num_nodes;
    int s = 0; 
    int cycle_count = 0;
    std::vector<int> active(G->num_nodes, 1); // todos ativos inicialmente


    while (s < n)
    {

        std::vector<std::vector<int>> SCCs = BFS_foward_backward_SCCs_v1(G, active, s);
        
        
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
            continue; // nenhuma SCC contém s, passa para o próximo s
        }


        // Cria unordered_set para busca O(1)
        std::unordered_set<int> scc_set(scc_vertices.begin(), scc_vertices.end());

        std::vector<bool> blocked(n, false);
        std::vector<std::unordered_set<int>> B(n);
        std::vector<int> stack;

        circuit_v1(s, s, G, scc_set, blocked, B, stack, cycle_count);
  

        active[s] = 0; // marca s como inativo ("remove" do grafo)
        ++s;
    }


    return cycle_count;
    
}