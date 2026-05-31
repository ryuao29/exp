#include "exp/bron_kerbosch.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <vector>

#ifdef USE_OPENMP
#include <omp.h>
#endif

namespace {

using mce::Clique;
using mce::Graph;

void normalize(std::vector<Clique>& cliques) {
  for (auto& clique : cliques) {
    std::sort(clique.begin(), clique.end());
  }
  std::sort(cliques.begin(), cliques.end());
}

}  // namespace

int main() {
  Graph graph(5);
  graph.add_edge(0, 1);
  graph.add_edge(0, 2);
  graph.add_edge(1, 2);
  graph.add_edge(2, 3);
  graph.add_edge(3, 4);
  graph.sort_and_unique();

#ifdef USE_OPENMP
  omp_set_num_threads(2);
#endif

  std::vector<Clique> cliques;
  mce::enumerate_maximal_cliques_no_pivot_task_parallel(graph, [&](const Clique& clique) {
    cliques.push_back(clique);
  }, 2);

  normalize(cliques);

  std::vector<Clique> expected{{0, 1, 2}, {2, 3}, {3, 4}};
  normalize(expected);

  assert(cliques == expected);
  return 0;
}