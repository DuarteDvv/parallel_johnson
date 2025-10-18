Nesta evolução, atacamos o custo de copiar estruturas por task e adicionamos um fallback sequencial para evitar paralelismo ineficiente em SCCs grandes.

O que mudou
- Estruturas mais compactas:
  - `scc_set` (unordered_set) virou `scc_mask` (`std::vector<char>`), tornando o teste de pertinência O(1) sem hashing e com melhor locality de cache.
  - `B` deixou de ser `std::unordered_set<int>` e passou a ser `std::vector<std::vector<int>>`, o que barateia cópias entre tasks e melhora a localidade.
- Desbloqueio adaptado: `unblock_v3` e o registro de dependências foram ajustados para `std::vector<int>`, evitando inserções duplicadas via checagem linear (listas de dependência tendem a ser curtas).
- Fallback sequencial por heurística:
  - Se a SCC de s ultrapassa `SCC_SEQUENTIAL_RATIO` (padrão 0.8) do total de vértices, usamos `circuit_v3_sequential` (sem tasks).
  - Evita spawn de muitas tasks com estados enormes quando a chance de ganho é baixa.

Por que melhora a performance em relação à v2
- Menor custo por task: copiar um vetor de bools e vetores de inteiros é significativamente mais barato que copiar vários hash sets; a criação e execução de tasks fica mais leve.
- Melhor uso de cache: estruturas contíguas (`vector`) favorecem prefetching e reduzem faltas de cache em relação a tabelas de hashing.
- Heurística de fallback: protege casos degenerados (SCCs enormes) nos quais o overhead de tarefas excede o trabalho útil, mantendo throughput previsível.

Limitações/Trade-offs
- A checagem de duplicatas em `B[w]` é O(k) no tamanho da lista; em grafos patológicos com `B[w]` muito grande, pode aumentar, mas na prática costuma ser pequeno.
- O valor de `SCC_SEQUENTIAL_RATIO` requer calibração; valores ruins podem desativar o paralelismo cedo demais ou, ao contrário, manter overhead excessivo.

Resultado prático esperado
- Speedup maior e mais estável do que v2 em grafos reais: tasks mais baratas + fallback inteligente tendem a entregar melhor relação custo/benefício do paralelismo.
