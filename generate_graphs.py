import networkx as nx
import random

def generate_directed_graph(
    num_nodes=6,
    p_edge=0.3,
    seed=42,
    filename="graph_snap.txt"
):
    """
    Gera um grafo direcionado aleatório e salva no formato SNAP.
    
    Parâmetros:
    ------------
    num_nodes : int
        Número de vértices do grafo.
    p_edge : float
        Probabilidade de existir uma aresta
    seed : int
        Semente do gerador aleatório 
    filename : str
        Nome do arquivo de saída.
    """

    random.seed(seed)
    G = nx.gnp_random_graph(num_nodes, p_edge, directed=True)
    G.remove_edges_from(nx.selfloop_edges(G))  # remove auto-laços

    num_edges = G.number_of_edges()

    with open(filename, "w") as f:
        f.write("# SNAP directed edge list: source target\n")
        f.write(f"# Nodes: {num_nodes} Edges: {num_edges}\n")
        f.write("# FromNodeId\tToNodeId\n")
        for u, v in G.edges():
            f.write(f"{u} {v}\n")

    print(f"Grafo gerado com {num_nodes} nós e {num_edges} arestas.")
    print(f"Arquivo salvo em: {filename}")


# ============================================================
# Exemplo de uso
# ============================================================
if __name__ == "__main__":
    # Configurações (você pode alterar)
    num_nodes = 15
    generate_directed_graph(
        num_nodes=num_nodes,   # número de nós
        p_edge=0.4,    # probabilidade de aresta
        seed=123,      # semente aleatória
        filename=f"graph_{num_nodes}.txt"
    )
