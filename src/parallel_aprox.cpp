// Parallel approximate Johnson: limits cycle length to max_len and parallelizes circuit
#include "parallel_aprox.hpp"
#include <omp.h>
#include <atomic>
#include <vector>
#include <unordered_set>
#include <algorithm>

#define PARALLEL_DEPTH 4
#define PARALLEL_BRANCH 2
#define DEBUG 0

// Reuse SCC decomposition from the sequential implementation
extern std::vector<std::vector<int>> BFS_foward_backward_SCCs(
	Graph G, const std::vector<int>& active, int min_vertex);

static void unblock_aprox_par(int u, std::vector<bool>& blocked, std::vector<std::vector<int>>& B) {
	blocked[u] = false;
	for (int w : B[u]) {
		if (blocked[w]) {
			unblock_aprox_par(w, blocked, B);
		}
	}
	B[u].clear();
}

// Parallel circuit with max_len pruning
static bool circuit_aprox_parallel(
	int v,
	int s,
	Graph G,
	const std::vector<char>& scc_mask,
	std::vector<bool>& blocked,
	std::vector<std::vector<int>>& B,
	int& cycle_count,
	int max_len,
	int curr_len,
	int depth = 0
) {
	bool found_cycle = false;
	blocked[v] = true;

	// gather neighbors inside SCC and >= s
	std::vector<int> neighbors;
	const Vertex* out_begin = outgoing_begin(G, v);
	const Vertex* out_end   = outgoing_end(G, v);
	neighbors.reserve(out_end - out_begin);
	for (const Vertex* it = out_begin; it != out_end; ++it) {
		int w = *it;
		if (w < s) continue;
		if (!scc_mask[w]) continue;
		neighbors.push_back(w);
	}

	const int branching = (int)neighbors.size();
	const bool allow_spawn = (depth < PARALLEL_DEPTH) && (branching >= PARALLEL_BRANCH);

	std::atomic<bool> any_child_found(false);

	#pragma omp taskgroup
	{
		for (int i = 0; i < branching; ++i) {
			int w = neighbors[i];
			int next_len = curr_len + 1;

			if (w == s) {
				if (next_len <= max_len) {
					#pragma omp atomic
					cycle_count++;
					found_cycle = true;
				}
			} else if (!blocked[w]) {
				if (next_len < max_len) {
					if (allow_spawn) {
						std::vector<bool> blocked_copy = blocked;
						std::vector<std::vector<int>> B_copy = B;
						#pragma omp task firstprivate(w, blocked_copy, B_copy, depth, next_len, max_len) shared(any_child_found, cycle_count, G, scc_mask)
						{
							bool child_res = circuit_aprox_parallel(w, s, G, scc_mask, blocked_copy, B_copy, cycle_count, max_len, next_len, depth + 1);
							if (child_res) any_child_found.store(true, std::memory_order_relaxed);
						}
					} else {
						if (circuit_aprox_parallel(w, s, G, scc_mask, blocked, B, cycle_count, max_len, next_len, depth + 1)) {
							found_cycle = true;
						}
					}
				}
			}
		}
	}

	if (any_child_found.load(std::memory_order_relaxed)) {
		found_cycle = true;
	}

	if (found_cycle) {
		unblock_aprox_par(v, blocked, B);
	} else {
		for (int w : neighbors) {
			auto& bucket = B[w];
			if (std::find(bucket.begin(), bucket.end(), v) == bucket.end()) {
				bucket.push_back(v);
			}
		}
	}

	return found_cycle;
}

int johnson_cycles_approx_parallel(Graph G, int max_len) {
	int n = G->num_nodes;
	int s = 0;
	int cycle_count = 0;
	std::vector<int> active(n, 1);

	while (s < n) {
		// Use the existing SCC finder from sequential version
		std::vector<std::vector<int>> SCCs = BFS_foward_backward_SCCs(G, active, s);

		std::vector<int> scc_vertices;
		for (const auto& scc : SCCs) {
			if (std::find(scc.begin(), scc.end(), s) != scc.end()) {
				scc_vertices = scc;
				break;
			}
		}

		if (scc_vertices.empty()) {
			active[s] = 0;
			++s;
			continue;
		}

		std::vector<char> scc_mask(n, 0);
		for (int v : scc_vertices) scc_mask[v] = 1;

		std::vector<bool> blocked(n, false);
		std::vector<std::vector<int>> B(n);

		// Parallel region spawning a single initial task tree
		#pragma omp parallel
		{
			#pragma omp single
			{
				circuit_aprox_parallel(s, s, G, scc_mask, blocked, B, cycle_count, max_len, 0, 0);
			}
		}

		active[s] = 0;
		++s;
	}

	return cycle_count;
}

