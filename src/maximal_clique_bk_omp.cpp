#include "exp/bron_kerbosch.hpp"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#ifdef USE_OPENMP
#include <omp.h>
#endif

using mce::Clique;
using mce::Graph;

namespace {

void print_clique(const Clique& clique) {
  for (std::size_t i = 0; i < clique.size(); ++i) {
    if (i != 0) {
      std::cout << ' ';
    }
    std::cout << clique[i];
  }
  std::cout << '\n';
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: " << argv[0] << " <graph.txt> [task_depth] [threads]\n";
    std::cerr << "task_depth controls how many shallow levels spawn OpenMP tasks\n";
    return 1;
  }

  const auto graph = Graph::load_from_file(argv[1]);
  const int task_depth_limit = argc >= 3 ? std::max(0, std::stoi(argv[2])) : 1;

#ifdef USE_OPENMP
  if (argc >= 4) {
    omp_set_num_threads(std::max(1, std::stoi(argv[3])));
  }
#endif

  std::size_t clique_count = 0;
  mce::enumerate_maximal_cliques_no_pivot_task_parallel(graph, [&](const Clique& clique) {
    ++clique_count;
    print_clique(clique);
  }, task_depth_limit);

  std::cout << "count=" << clique_count << '\n';
  return 0;
}