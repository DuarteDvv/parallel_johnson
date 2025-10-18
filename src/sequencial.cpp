#include "sequencial.hpp"


#define DEBUG 0

std::vector<std::vector<int>> BFS_foward_backward_SCCs(
    Graph G, // grafo
    const std::vector<int>& active, // vetor indicando se vértice está ativo (1) ou inativo (0) (ao invés de remover do grafo, marca como inativo)
    int min_vertex // menor vértice a considerar (todos < min_vertex são inativos)
){
    
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

void unblock(
    int u, // vértice a desbloquear
    std::vector<bool>& blocked, // vetor de bloqueados
    std::vector<std::unordered_set<int>>& B // listas de dependência
) {
    blocked[u] = false;
    for (int w : B[u]) {
        if (blocked[w]) {
            unblock(w, blocked, B);
        }
    }
    B[u].clear();
}


bool circuit(
    int v, // vértice atual da recursão (onde está)
    int s, // vértice de origem (onde começou a busca e deve terminar o ciclo)
    Graph G, // grafo
    const std::unordered_set<int>& scc_set, // conjunto de vértices da SCC atual; qualquer vértice fora desse conjunto é ignorado.
    std::vector<bool>& blocked, // vetor de vértices bloqueados (dependência)
    std::vector<std::unordered_set<int>>& B, // vetor de conjuntos; B[w] armazena vértices que devem ser desbloqueados se w for desbloqueado. (dependência)
    int& cycle_count // contador de ciclos encontrados (dependência)
) {

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
            if (circuit(w, s, G, scc_set, blocked, B, cycle_count)) {
                found_cycle = true;
            }
        }
    }

    if (found_cycle) {
        unblock(v, blocked, B);
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

int johnson_cycles(
    Graph G // grafo
) {

    int n = G->num_nodes;
    int s = 0; 
    int cycle_count = 0;
    std::vector<int> active(G->num_nodes, 1); // todos ativos inicialmente

    double SCC_time = 0.0;
    double circuit_time = 0.0;
    

    while (s < n)
    {

        double startSCC = CycleTimer::currentSeconds();

        std::vector<std::vector<int>> SCCs = BFS_foward_backward_SCCs(G, active, s);

        double endSCC = CycleTimer::currentSeconds();
        SCC_time += (endSCC - startSCC);

        if (DEBUG){
            std::cout << "Number of SCCs: " << SCCs.size() << std::endl;
        }
        
        
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

        if (DEBUG){
            std::cout << "SCC containing " << s << ": ";
            for (int v : scc_vertices) {
                std::cout << v << " ";
            }
            std::cout << std::endl;
        }

        // Cria unordered_set para busca O(1)
        std::unordered_set<int> scc_set(scc_vertices.begin(), scc_vertices.end());

        std::vector<bool> blocked(n, false);
        std::vector<std::unordered_set<int>> B(n);
    // removido: stack

        double startCircuit = CycleTimer::currentSeconds();
    circuit(s, s, G, scc_set, blocked, B, cycle_count);
        double endCircuit = CycleTimer::currentSeconds();
        circuit_time += (endCircuit - startCircuit);

        active[s] = 0; // marca s como inativo ("remove" do grafo)
        ++s;
    }

    printf("Total SCC time: %.6f seconds\n", SCC_time);
    printf("Total circuit time: %.6f seconds\n", circuit_time);


    return cycle_count;
    
}