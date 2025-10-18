Exploramos paralelismo de grão grosso no laço externo do Johnson (sobre s), explorando o fato de que cada s é independente após “remover” vértices anteriores.

Paralelização do laço de s: uso de OpenMP com parallel + for (schedule(dynamic, 1)) para distribuir diferentes valores de s entre as threads.
Isolamento de estado por thread:
- Cada thread cria uma cópia local de active e marca [0..s-1] como inativos (evita sincronização).
- Contador de ciclos local por thread; no final, soma no total com operação atômica.
- Fluxo interno intacto: para cada s, ainda se calcula a SCC por BFS forward/backward e roda a busca de ciclo de forma sequencial (circuit) — a diferença é que várias instâncias disso acontecem em paralelo, uma por s.

Problemas:

Por mais que cada s seja independente, quanto maior o s, menos nós vão ter o subgrafo (SCC) em que serão feitas as operações de circuit que é a mais demorada. O resultado disso é que as primeiras threads que pegarem s pequenos serão um gargalo pois terão o maior grafo (inteiro) e portanto um tempo exponencialmente maior de execução. Ou seja, temos um cenário de desbalanceamento de carga que leva a necessidade buscar estratégias para paralelizar a função circuit.