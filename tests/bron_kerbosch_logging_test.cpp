#include "exp/bron_kerbosch.hpp"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

int main() {
  const std::string path = "/tmp/bron_kerbosch_logging_test.csv";
  std::remove(path.c_str());

  mce::Graph graph(5);
  graph.add_edge(0, 1);
  graph.add_edge(0, 2);
  graph.add_edge(1, 2);
  graph.add_edge(2, 3);
  graph.add_edge(3, 4);
  graph.sort_and_unique();

  mce::RecursionTreeCsvLogger logger(path);

  std::vector<mce::Clique> cliques;
  mce::enumerate_maximal_cliques_no_pivot(graph,
                                          [&](const mce::Clique& clique) { cliques.push_back(clique); },
                                          &logger);
  logger.flush();

  assert(cliques.size() == 3);

  std::ifstream in(path);
  assert(in);

  std::string line;
  std::size_t line_count = 0;
  bool saw_header = false;
  bool saw_data = false;
  while (std::getline(in, line)) {
    ++line_count;
    if (line_count == 1) {
      saw_header = line == "node_id,parent_id,depth,clique_size,p_size,x_size,candidate_count,child_count,elapsed_us,is_leaf";
    } else {
      saw_data = true;
    }
  }

  assert(saw_header);
  assert(saw_data);
  assert(line_count >= 2);

  std::remove(path.c_str());
  return 0;
}