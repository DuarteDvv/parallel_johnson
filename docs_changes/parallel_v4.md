Nesta evolução, protegemos a paralelização contra casos densos (grafos quase completos) e reduzimos a contenção/overhead de tarefas, mantendo o pruning do Johnson efetivo.

O que mudou
- Paralelismo só no topo do circuito:
	- Criação de tasks apenas em `depth == 0` (nível raiz). A exploração abaixo do topo roda inline/sequencialmente, evitando cascatas de tarefas profundas.
	- Cada task de topo usa cópias locais de `blocked` e `B`, preservando o mecanismo de bloqueio/desbloqueio do Johnson sem sincronização fina.
- Heurística por branching (grau de expansão):
	- `PARALLEL_MIN_BRANCH` e `PARALLEL_MAX_BRANCH` definem uma janela em que vale a pena spawnar tasks.
	- Se o branching do nó raiz for maior que `PARALLEL_MAX_BRANCH`, caímos para o Johnson sequencial naquele ramo (evita explosão em grafos completos/densos).
- Menos contenção no contador de ciclos:
	- Acúmulo local de ciclos por bloco/tarefa e apenas um `atomic` no final (em vez de `atomic` a cada ciclo encontrado).
- Reuso de otimizações anteriores:
	- Mantemos `scc_mask` como `std::vector<char>` e `B` como `std::vector<std::vector<int>>` (v3), além da reserva prévia de vizinhos e uso de `#pragma omp taskgroup` com `std::atomic<bool>` (v2/v3).

Parâmetros/flags novos ou ajustados
- `PARALLEL_MIN_BRANCH` (padrão: 4): grau mínimo para justificar criação de tasks no topo.
- `PARALLEL_MAX_BRANCH` (padrão: 64): teto de grau; acima disso, força execução sequencial no topo para evitar overhead em grafos muito densos.
- `PARALLEL_TOP_LEVEL_ONLY` (padrão: 1): efetivamente aplicamos spawn apenas em `depth == 0`.
- `SCC_SEQUENTIAL_RATIO` (mantido): fallback para execução sequencial quando a SCC de s é muito grande em relação ao grafo inteiro.

Por que melhora a performance em relação à v3
- Evita a explosão de tarefas em grafos completos: limitar o spawn ao topo e impor um teto de branching elimina o custo de alocação/agendamento que superava o trabalho útil.
- Mantém o pruning do Johnson eficaz: ao executar cada task com estado local e evitar spawns profundos, reduzimos reexploração e preservamos o desbloqueio correto.
- Menos atomics, mais throughput: consolidar contagens localmente diminui a contenção do contador global em instâncias com muitos ciclos.

Limitações/Trade-offs
- Paralelismo mais conservador: ao restringir ao topo, abrimos mão de alguma paralelização em ramos profundos (benefício líquido em densos, neutro ou levemente negativo em alguns esparsos).
- Cópias de `blocked/B` ainda existem nas tasks de topo: custo amortizado por serem poucas tasks e por estruturas contíguas baratas de copiar.
- Thresholds podem requerer tuning conforme número de threads e perfil do grafo (valores padrão focam estabilidade em densos).

Resultado prático esperado
- Speedup mais estável, com grande melhora em grafos densos/quase completos (onde v3 podia regredir por overhead). Em grafos moderados, desempenho semelhante ou um pouco melhor que v3 graças à redução de contenção e coordenação.

