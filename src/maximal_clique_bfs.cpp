#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>
#ifdef USE_OPENMP
#include <omp.h>
#endif

using Graph = std::vector<std::unordered_set<int>>;

Graph load_graph(const std::string& path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("failed to open graph file: " + path);
  }

  int n = 0;
  in >> n;
  Graph g(static_cast<size_t>(n));

  int u = 0;
  int v = 0;
  while (in >> u >> v) {
    if (u < 0 || v < 0 || u >= n || v >= n || u == v) {
      continue;
    }
    g[static_cast<size_t>(u)].insert(v);
    g[static_cast<size_t>(v)].insert(u);
  }
  return g;
}

std::vector<int> intersect_candidates(const std::vector<int>& candidates,
                                      const std::unordered_set<int>& neigh) {
  std::vector<int> out;
  out.reserve(candidates.size());
  for (int c : candidates) {
    if (neigh.find(c) != neigh.end()) {
      out.push_back(c);
    }
  }
  return out;
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "usage: " << argv[0] << " <graph.txt> <depth> [threads]\n";
    std::cerr << "graph format: first line is vertex count N, then each line is edge u v\n";
    return 1;
  }

  const auto graph = load_graph(argv[1]);
  const int depth_limit = std::max(0, std::stoi(argv[2]));
#ifdef USE_OPENMP
  if (argc >= 4) {
    omp_set_num_threads(std::max(1, std::stoi(argv[3])));
  }
#endif

  std::vector<std::vector<int>> frontier(1);

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
        for (int candidate = last + 1; candidate < static_cast<int>(graph.size()); ++candidate) {
          bool ok = true;
          for (int v : clique) {
            if (graph[static_cast<size_t>(v)].find(candidate) == graph[static_cast<size_t>(v)].end()) {
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
      for (int candidate = last + 1; candidate < static_cast<int>(graph.size()); ++candidate) {
        bool ok = true;
        for (int v : clique) {
          if (graph[static_cast<size_t>(v)].find(candidate) == graph[static_cast<size_t>(v)].end()) {
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
    if (frontier.empty()) {
      break;
    }
  }

  std::cout << "finished BFS-style expansion. max explored clique size=" << max_clique_size_seen << '\n';
  return 0;
}
