#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "exp/graph.hpp"
#include "exp/logger.hpp"
#ifdef USE_OPENMP
#include <omp.h>
#endif

using mce::CsvLogger;
using mce::Graph;

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "usage: " << argv[0] << " <graph.txt> <depth> [threads]\n";
    std::cerr << "graph format: first line is vertex count N, then each line is edge u v\n";
    return 1;
  }

  const auto graph = Graph::load_from_file(argv[1]);
  const int depth_limit = std::max(0, std::stoi(argv[2]));
#ifdef USE_OPENMP
  if (argc >= 4) {
    omp_set_num_threads(std::max(1, std::stoi(argv[3])));
  }
#endif

  std::vector<std::vector<int>> frontier(1);
  std::unique_ptr<CsvLogger> logger;
  if (argc >= 5) {
    logger = std::make_unique<CsvLogger>(argv[4]);
    logger->write_header({"event", "depth", "frontier"});
  }

  size_t max_clique_size_seen = 0;
  for (int depth = 0; depth < depth_limit; ++depth) {
    std::vector<std::vector<int>> next_frontier;
#ifdef USE_OPENMP
#pragma omp parallel
    {
      std::vector<std::vector<int>> local_next;
#pragma omp for nowait
      for (size_t i = 0; i < frontier.size(); ++i) {
        const auto& clique = frontier[i];
        const int last = clique.empty() ? -1 : clique.back();
        for (int candidate = last + 1; candidate < static_cast<int>(graph.vertex_count()); ++candidate) {
          bool ok = true;
          for (int v : clique) {
            if (!graph.has_edge(static_cast<std::size_t>(v), static_cast<std::size_t>(candidate))) {
              ok = false;
              break;
            }
          }
          if (ok) {
            auto extended = clique;
            extended.push_back(candidate);
            local_next.push_back(std::move(extended));
          }
        }
      }
#pragma omp critical
      next_frontier.insert(next_frontier.end(), local_next.begin(), local_next.end());
    }
#else
    for (const auto& clique : frontier) {
      const int last = clique.empty() ? -1 : clique.back();
      for (int candidate = last + 1; candidate < static_cast<int>(graph.vertex_count()); ++candidate) {
        bool ok = true;
        for (int v : clique) {
          if (!graph.has_edge(static_cast<std::size_t>(v), static_cast<std::size_t>(candidate))) {
            ok = false;
            break;
          }
        }
        if (ok) {
          auto extended = clique;
          extended.push_back(candidate);
          next_frontier.push_back(std::move(extended));
        }
      }
    }
#endif

    frontier = std::move(next_frontier);
    max_clique_size_seen = std::max(max_clique_size_seen, static_cast<size_t>(depth + 1));

    std::cout << "depth=" << (depth + 1) << " frontier=" << frontier.size() << '\n';
    if (logger) {
      logger->write_row({"depth", std::to_string(depth + 1), std::to_string(frontier.size())});
    }
    if (frontier.empty()) {
      break;
    }
  }

  std::cout << "finished BFS-style expansion. max explored clique size=" << max_clique_size_seen << '\n';
  return 0;
}
