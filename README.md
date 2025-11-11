# Parallel Johnson's Algorithm

Implementação paralela do algoritmo de Johnson para encontrar todos os ciclos simples em um grafo direcionado.

## Requisitos

- g++ com suporte a C++17
- OpenMP
- make

## Compilação

Para compilar o projeto, execute:

```bash
make
```

O executável será gerado em `bin/sccs`.

Para limpar os arquivos compilados:

```bash
make clean
```

## Uso

### Sintaxe Básica

```bash
./bin/sccs [-v N] [-s] [-e] <caminho/para/arquivo/grafo> [num_threads]
```

### Opções

- `-v N`: Especifica qual versão executar
  - `0`: Apenas versão sequencial
  - `1`: Sequencial + Parallel v0 (paralelização do loop principal)
  - `2`: Sequencial + Parallel v1 (usando tasks)
  - `3`: Sequencial + Parallel v2 (usando taskgroup)
  - `4`: Sequencial + Parallel v3 (abordagem híbrida)
  - `5`: Sequencial + Parallel v4 (window spawn)
  - Se omitido, executa todas as versões

- `-s`: Carrega o grafo a partir de arquivo texto (por padrão, carrega em formato binário)

- `-e`: Modo de avaliação - executa cada versão 5 vezes, descarta o menor e maior tempo, e calcula a média dos 3 tempos intermediários

- `num_threads`: Número de threads a usar (opcional, se omitido usa o máximo disponível no sistema)

### Exemplos de Uso

1. **Executar todas as versões com o máximo de threads:**
   ```bash
   ./bin/sccs datasets/tiny.graph
   ```

2. **Executar todas as versões com 4 threads:**
   ```bash
   ./bin/sccs datasets/tiny.graph 4
   ```

3. **Executar apenas a versão sequencial:**
   ```bash
   ./bin/sccs -v 0 datasets/tiny.graph
   ```

4. **Executar a versão parallel v2 com 8 threads:**
   ```bash
   ./bin/sccs -v 3 datasets/tiny.graph 8
   ```

5. **Carregar grafo de arquivo texto:**
   ```bash
   ./bin/sccs -s datasets/texts/test.txt
   ```

6. **Carregar grafo de arquivo texto e executar versão v1:**
   ```bash
   ./bin/sccs -v 2 -s datasets/texts/graph_15.txt 4
   ```

7. **Modo de avaliação - executar 5 vezes e calcular média:**
   ```bash
   ./bin/sccs -e datasets/tiny.graph
   ```

8. **Modo de avaliação com versão específica e threads definidas:**
   ```bash
   ./bin/sccs -e -v 5 datasets/graph_17.txt.bin 12
   ```

## Formatos de Arquivo

### Formato Binário (.graph)
Formato otimizado para carregamento rápido (padrão).

### Formato Texto (.txt)
Arquivo de texto com a representação do grafo. Use a opção `-s` para carregar este formato.

## Saída

O programa exibe:
- Número máximo de threads do sistema
- Número de threads utilizadas
- Estatísticas do grafo (número de arestas e nós)
- Para cada versão executada:
  - Tempo de execução
  - Número de ciclos simples encontrados
  - Speedup (para versões paralelas)

## Versões Paralelas

- **v0**: Paralelização do loop principal usando `#pragma omp parallel for`
- **v1**: Paralelização usando tasks OpenMP
- **v2**: Paralelização usando taskgroup OpenMP
- **v3**: Abordagem híbrida combinando diferentes estratégias
- **v4**: Estratégia de window spawn para melhor balanceamento de carga

## Estrutura do Projeto

```
.
├── bin/                  # Executáveis compilados
├── build/                # Arquivos objeto intermediários
├── common/               # Código comum (grafo, utilitários)
├── datasets/             # Conjuntos de dados de teste
├── docs_changes/         # Documentação das versões paralelas
├── include/              # Headers das implementações
├── src/                  # Código fonte das implementações
├── Makefile              # Script de compilação
└── README.md             # Este arquivo
```
