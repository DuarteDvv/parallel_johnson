#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <unordered_set>
#include <algorithm>

#include "graph.h"
#include "graph_internal.h"

#define GRAPH_HEADER_TOKEN ((int) 0xDEADBEEF)


void free_graph(Graph graph)
{
  free(graph->outgoing_starts);
  free(graph->outgoing_edges);

  free(graph->incoming_starts);
  free(graph->incoming_edges);
  free(graph);
}


void build_start(graph* graph, int* scratch)
{
  int num_nodes = graph->num_nodes;
  graph->outgoing_starts = (int*)malloc(sizeof(int) * num_nodes);
  for(int i = 0; i < num_nodes; i++)
  {
    graph->outgoing_starts[i] = scratch[i];
  }
}

void build_edges(graph* graph, int* scratch)
{
  int num_nodes = graph->num_nodes;
  graph->outgoing_edges = (int*)malloc(sizeof(int) * graph->num_edges);
  for(int i = 0; i < graph->num_edges; i++)
  {
    graph->outgoing_edges[i] = scratch[num_nodes + i];
  }
}

// Given an outgoing edge adjacency list representation for a directed
// graph, build an incoming adjacency list representation
void build_incoming_edges(graph* graph) {

    //printf("Beginning build_incoming... (%d nodes)\n", graph->num_nodes);

    int num_nodes = graph->num_nodes;
    int* node_counts = (int*)malloc(sizeof(int) * num_nodes);
    int* node_scatter = (int*)malloc(sizeof(int) * num_nodes);

    graph->incoming_starts = (int*)malloc(sizeof(int) * num_nodes);
    graph->incoming_edges = (int*)malloc(sizeof(int) * graph->num_edges);

    for (int i=0; i<num_nodes; i++)
        node_counts[i] = node_scatter[i] = 0;

    int total_edges = 0;
    // compute number of incoming edges per node
    for (int i=0; i<num_nodes; i++) {
        int start_edge = graph->outgoing_starts[i];
        int end_edge = (i == graph->num_nodes-1) ? graph->num_edges : graph->outgoing_starts[i+1];
        for (int j=start_edge; j<end_edge; j++) {
            int target_node = graph->outgoing_edges[j];
            node_counts[target_node]++;
            total_edges++;
        }
    }
    //printf("Total edges: %d\n", total_edges);
    //printf("Computed incoming edge counts.\n");

    // build the starts array
    graph->incoming_starts[0] = 0;
    for (int i=1; i<num_nodes; i++) {
        graph->incoming_starts[i] = graph->incoming_starts[i-1] + node_counts[i-1];
        //printf("%d: %d ", i, graph->incoming_starts[i]);
    }
    //printf("\n");
    //printf("Last edge=%d\n", graph->incoming_starts[num_nodes-1] + node_counts[num_nodes-1]);

    //printf("Computed per-node incoming starts.\n");

    // now perform the scatter
    for (int i=0; i<num_nodes; i++) {
        int start_edge = graph->outgoing_starts[i];
        int end_edge = (i == graph->num_nodes-1) ? graph->num_edges : graph->outgoing_starts[i+1];
        for (int j=start_edge; j<end_edge; j++) {
            int target_node = graph->outgoing_edges[j];
            graph->incoming_edges[graph->incoming_starts[target_node] + node_scatter[target_node]] = i;
            node_scatter[target_node]++;
        }
    }

    /*
    // verify
    printf("Verifying graph...\n");

    for (int i=0; i<num_nodes; i++) {
        int outgoing_starts = graph->outgoing_starts[i];
        int end_node = (i == graph->num_nodes-1) ? graph->num_edges : graph->outgoing_starts[i+1];
        for (int j=outgoing_starts; j<end_node; j++) {

            bool verified = false;

            // make sure that i is a neighbor of target_node
            int target_node = graph->outgoing_edges[j];
            int j_start_edge = graph->incoming_starts[target_node];
            int j_end_edge = (target_node == graph->num_nodes-1) ? graph->num_edges : graph->incoming_starts[target_node+1];
            for (int k=j_start_edge; k<j_end_edge; k++) {
                if (graph->incoming_edges[k] == i) {
                    verified = true;
                    break;
                }
            }

            if (!verified) {
                fprintf(stderr,"Error: %d,%d did not verify\n", i, target_node);
            }
        }
    }

    printf("Done verifying\n");
    */

    free(node_counts);
    free(node_scatter);
}

void get_meta_data(std::ifstream& file, graph* graph)
{
    file.clear();
    file.seekg(0, std::ios::beg);

    std::string buffer;
    graph->num_nodes = 0;
    graph->num_edges = 0;

    while (std::getline(file, buffer)) {
        if (buffer.size() == 0) continue;
        if (buffer[0] != '#') {
            // Achamos a primeira linha de dados, voltar o ponteiro
            file.seekg(-((std::streamoff)buffer.size()) - 1, std::ios::cur);
            break;
        }

        // Tenta capturar "# Nodes: X Edges: Y"
        if (buffer.find("Nodes:") != std::string::npos) {
            sscanf(buffer.c_str(), "# Nodes: %d Edges: %d",
                   &graph->num_nodes, &graph->num_edges);
        }
    }
}

// ----------------------------------------------------------
// Lê arquivo SNAP (pares u v por linha) e constrói CSR
// ----------------------------------------------------------
void read_graph_file(std::ifstream& file, int* scratch)
{
    file.clear();
    file.seekg(0, std::ios::beg);

    std::string buffer;
    std::vector<std::pair<int,int>> edges;

    // Lê todas as arestas u v
    while (std::getline(file, buffer)) {
        if (buffer.size() == 0 || buffer[0] == '#')
            continue;

        std::stringstream ss(buffer);
        int u, v;
        if (ss >> u >> v)
            edges.emplace_back(u, v);
    }

    // Determina número de nós (caso não tenha sido lido antes)
    int max_node = 0;
    for (auto& e : edges)
        max_node = std::max({max_node, e.first, e.second});

    int num_nodes = max_node + 1;
    int num_edges = (int)edges.size();

    // Vetor auxiliar de graus
    std::vector<int> out_degree(num_nodes, 0);
    for (auto& e : edges)
        out_degree[e.first]++;

    // Constrói o array starts
    std::vector<int> starts(num_nodes, 0);
    for (int i = 1; i < num_nodes; i++)
        starts[i] = starts[i-1] + out_degree[i-1];

    // Constrói o array edges (lista concatenada)
    std::vector<int> edge_targets(num_edges);
    std::vector<int> offset(num_nodes, 0);

    for (auto& e : edges) {
        int u = e.first;
        int v = e.second;
        int pos = starts[u] + offset[u]++;
        edge_targets[pos] = v;
    }

    // Copia tudo para "scratch"
    // (Primeiro starts[], depois edges[])
    for (int i = 0; i < num_nodes; i++)
        scratch[i] = starts[i];
    for (int i = 0; i < num_edges; i++)
        scratch[num_nodes + i] = edge_targets[i];
}


void print_graph(const graph* graph)
{

    printf("Graph pretty print:\n");
    printf("num_nodes=%d\n", graph->num_nodes);
    printf("num_edges=%d\n", graph->num_edges);

    for (int i=0; i<graph->num_nodes; i++) {

        int start_edge = graph->outgoing_starts[i];
        int end_edge = (i == graph->num_nodes-1) ? graph->num_edges : graph->outgoing_starts[i+1];
        printf("node %02d: out=%d: ", i, end_edge - start_edge);
        for (int j=start_edge; j<end_edge; j++) {
            int target = graph->outgoing_edges[j];
            printf("%d ", target);
        }
        printf("\n");

        start_edge = graph->incoming_starts[i];
        end_edge = (i == graph->num_nodes-1) ? graph->num_edges : graph->incoming_starts[i+1];
        printf("         in=%d: ", end_edge - start_edge);
        for (int j=start_edge; j<end_edge; j++) {
            int target = graph->incoming_edges[j];
            printf("%d ", target);
        }
        printf("\n");
    }
}

Graph load_graph(const char* filename)
{
  graph* graph = (struct graph*)(malloc(sizeof(struct graph)));

  // open the file
  std::ifstream graph_file;
  graph_file.open(filename);
  get_meta_data(graph_file, graph);

  int* scratch = (int*) malloc(sizeof(int) * (graph->num_nodes + graph->num_edges));
  read_graph_file(graph_file, scratch);

  build_start(graph, scratch);
  build_edges(graph, scratch);
  free(scratch);

  build_incoming_edges(graph);

  //print_graph(graph);

  return graph;
}

Graph load_graph_binary(const char* filename)
{
    graph* graph = (struct graph*)(malloc(sizeof(struct graph)));

    FILE* input = fopen(filename, "rb");

    if (!input) {
        fprintf(stderr, "Could not open: %s\n", filename);
        exit(1);
    }

    int header[3];

    if (fread(header, sizeof(int), 3, input) != 3) {
        fprintf(stderr, "Error reading header.\n");
        exit(1);
    }

    if (header[0] != GRAPH_HEADER_TOKEN) {
        fprintf(stderr, "Invalid graph file header. File may be corrupt.\n");
        exit(1);
    }

    graph->num_nodes = header[1];
    graph->num_edges = header[2];

    graph->outgoing_starts = (int*)malloc(sizeof(int) * graph->num_nodes);
    graph->outgoing_edges = (int*)malloc(sizeof(int) * graph->num_edges);

    if (fread(graph->outgoing_starts, sizeof(int), graph->num_nodes, input) != (size_t) graph->num_nodes) {
        fprintf(stderr, "Error reading nodes.\n");
        exit(1);
    }

    if (fread(graph->outgoing_edges, sizeof(int), graph->num_edges, input) != (size_t) graph->num_edges) {
        fprintf(stderr, "Error reading edges.\n");
        exit(1);
    }

    fclose(input);

    build_incoming_edges(graph);
    //print_graph(graph);
    return graph;
}

void store_graph_binary(const char* filename, Graph graph) {

    FILE* output = fopen(filename, "wb");

    if (!output) {
        fprintf(stderr, "Could not open: %s\n", filename);
        exit(1);
    }

    int header[3];
    header[0] = GRAPH_HEADER_TOKEN;
    header[1] = graph->num_nodes;
    header[2] = graph->num_edges;

    if (fwrite(header, sizeof(int), 3, output) != 3) {
        fprintf(stderr, "Error writing header.\n");
        exit(1);
    }

    if (fwrite(graph->outgoing_starts, sizeof(int), graph->num_nodes, output) != (size_t) graph->num_nodes) {
        fprintf(stderr, "Error writing nodes.\n");
        exit(1);
    }

    if (fwrite(graph->outgoing_edges, sizeof(int), graph->num_edges, output) != (size_t) graph->num_edges) {
        fprintf(stderr, "Error writing edges.\n");
        exit(1);
    }

    fclose(output);
}
