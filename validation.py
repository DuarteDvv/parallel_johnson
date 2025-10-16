import networkx as nx

def count_strongly_connected_components(file_path):
    """
    Conta o número de componentes fortemente conectados em um grafo
    a partir de um arquivo no formato SNAP.
    
    Args:
        file_path: Caminho para o arquivo no formato SNAP
    
    Returns:
        Número de componentes fortemente conectados
    """
    # Cria um grafo direcionado
    G = nx.DiGraph()
    
    # Lê o arquivo no formato SNAP
    with open(file_path, 'r') as f:
        for line in f:
            # Ignora linhas de comentário (começam com #)
            if line.startswith('#'):
                continue
            
            # Lê as arestas (formato: node1 node2)
            parts = line.strip().split()
            if len(parts) >= 2:
                source = int(parts[0])
                target = int(parts[1])
                G.add_edge(source, target)
    
    # Calcula os componentes fortemente conectados
    scc = list(nx.strongly_connected_components(G))
    
    # Imprime informações
    print(f"Número de vértices: {G.number_of_nodes()}")
    print(f"Número de arestas: {G.number_of_edges()}")
    print(f"Número de componentes fortemente conectados: {len(scc)}")
    
    # Imprime tamanho dos maiores componentes
    scc_sizes = sorted([len(c) for c in scc], reverse=True)
    print(f"\nTamanhos dos 5 maiores componentes: {scc_sizes[:5]}")
    
    return len(scc)

if __name__ == "__main__":
    # Substitua pelo caminho do seu arquivo
    file_path = "datasets/Amazon0302.txt"
    
    try:
        count_strongly_connected_components(file_path)
    except FileNotFoundError:
        print(f"Erro: Arquivo '{file_path}' não encontrado.")
    except Exception as e:
        print(f"Erro ao processar o arquivo: {e}")