Nesta evolução, levamos o paralelismo para dentro da rotina de enumeração de ciclos (circuit), eliminando a limitação da v0 que só paralelizava o laço externo sobre s.

O que mudou
- Introdução de tasks OpenMP dentro de circuit (circuit_v1_parallel): cada aresta de expansão potencial pode virar uma task, quando compensa.
- Política de criação de tarefas (heurística):
  - PARALLEL_DEPTH: limita a profundidade máxima em que novas tasks são criadas (evita explosão de tarefas muito profundas).
  - PARALLEL_BRANCH: requer um grau mínimo de ramificação para justificar o spawn (evita overhead quando há poucos vizinhos).
- Isolamento por cópia: ao spawnar, copiamos blocked e B para a task, garantindo independência de estados e eliminando sincronizações finas (locks) na estrutura de dependências.
- Sinalização de resultados: um vetor compartilhado child_found informa ao pai, após #pragma omp taskwait, se algum filho encontrou ciclo para realizar o desbloqueio correto.

Por que melhora a performance em relação à v0
- Paralelismo intrínseco do circuito: mesmo quando só existe um s “pesado”, a exploração da SCC é dividida entre vários núcleos, aliviando o gargalo das primeiras iterações pesadas observado na v0.
- Balanceamento natural: ramos com alto branching geram mais tasks e ocupam as threads, enquanto ramos leves são executados inline (sem overhead de task), melhorando a utilização.
- Overhead controlado: os thresholds evitam criar tasks em partes pouco promissoras da busca, mantendo o custo de agendamento e cópia sob controle.

Limitações/Trade-offs
- Custo de cópia por task: replicar blocked e B pode ser caro em SCCs grandes; ainda assim, é mais barato do que sincronizar estados compartilhados a cada passo da DFS.
- Granularidade variável: thresholds precisam de tuning por tipo de grafo; valores ruins podem sub ou superexplorar o paralelismo.

Resultado prático esperado
- Speedup superior ao da v0 em grafos onde poucas iterações de s concentram a maior parte do trabalho (componentes grandes), pois a parte dominante (circuit) agora escala com o número de núcleos.
