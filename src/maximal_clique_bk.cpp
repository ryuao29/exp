#include "exp/bron_kerbosch.hpp"

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

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
    std::cerr << "usage: " << argv[0] << " <graph.txt> [log.csv] [--show-clique]\n";
    std::cerr << "prints each maximal clique on one line; optional CSV recursion log can include clique vertices\n";
    return 1;
  }

  const auto graph = Graph::load_from_file(argv[1]);

  const bool enable_log = argc >= 3;
  const bool show_clique_vertices = argc >= 4 && std::string(argv[3]) == "--show-clique";

  std::unique_ptr<mce::RecursionTreeCsvLogger> logger;
  if (enable_log) {
    logger = std::make_unique<mce::RecursionTreeCsvLogger>(argv[2], true, show_clique_vertices);
  }

  std::size_t clique_count = 0;
  mce::enumerate_maximal_cliques_no_pivot(graph, [&](const Clique& clique) {
    ++clique_count;
    print_clique(clique);
  }, logger.get());

  if (logger) {
    logger->flush();
  }

  std::cout << "count=" << clique_count << '\n';
  return 0;
}