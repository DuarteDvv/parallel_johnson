Nesta evolução, reduzimos o overhead das tasks introduzidas na v1, atacando sincronizações e alocações desnecessárias para melhorar o speedup em grafos com muitos ramos.

O que mudou
- Taskgroup e sinalização atômica:
  - Substituímos o vetor compartilhado child_found + taskwait por `#pragma omp taskgroup` com um `std::atomic<bool> any_child_found`.
  - Cada tarefa seta `any_child_found` quando encontra ciclo, e o pai lê ao final do taskgroup. Menos memória temporária e sincronizações explícitas.
- Menos realocações em vizinhos: pré-reserva do vetor de vizinhos (`neighbors.reserve`) a partir do grau de saída, reduzindo realocação e cópia durante a coleta.
- Mesma política de spawn: mantivemos `PARALLEL_DEPTH` e `PARALLEL_BRANCH` para evitar explosão de tarefas em profundidades grandes ou com baixo branching.

Por que melhora a performance em relação à v1
- Menos sincronização fina: a troca por uma flag atômica e o uso de taskgroup diminuem a necessidade de estruturas auxiliares e `taskwait` + varreduras, reduzindo tempo ocioso.
- Menos pressão de memória: o desaparecimento do vetor child_found e a redução de realocações baixam o custo por task, permitindo criar mais tarefas úteis antes do ponto de retorno decrescente.
- Melhor escalabilidade: em grafos densos, onde o número de tasks cresce rapidamente, o custo fixo menor por task se traduz em speedup maior.

Limitações/Trade-offs
- O custo de copiar `blocked` e `B` por task permanece — ainda é a estratégia para evitar contenção; o ganho vem de sincronizar menos e alocar menos "sinalizadores".
- Os thresholds continuam precisando de tuning conforme o tipo de grafo e o número de threads.

Resultado prático esperado
- Speedup consistente maior que v1, especialmente em instâncias com branching alto (muitas expansões), onde a sobrecarga de coordenação da v1 era perceptível.
