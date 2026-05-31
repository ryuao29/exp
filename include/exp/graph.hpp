#pragma once

#include "exp/bitset.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mce {

class Graph {
public:
  Graph() = default;

  explicit Graph(std::size_t vertex_count) : adjacency_(vertex_count) {}

  static Graph load_from_file(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
      throw std::runtime_error("failed to open graph file: " + path);
    }

    std::size_t vertex_count = 0;
    in >> vertex_count;
    Graph graph(vertex_count);

    std::size_t u = 0;
    std::size_t v = 0;
    while (in >> u >> v) {
      graph.add_edge(u, v);
    }
    graph.sort_and_unique();
    return graph;
  }

  std::size_t vertex_count() const { return adjacency_.size(); }

  const std::vector<std::size_t>& neighbors(std::size_t vertex) const { return adjacency_.at(vertex); }

  std::size_t degree(std::size_t vertex) const { return adjacency_.at(vertex).size(); }

  bool has_edge(std::size_t u, std::size_t v) const {
    const auto& neigh = adjacency_.at(u);
    return std::binary_search(neigh.begin(), neigh.end(), v);
  }

  Bitset neighbor_bitset(std::size_t vertex) const {
    Bitset bits(vertex_count());
    for (auto neighbor : adjacency_.at(vertex)) {
      bits.set(neighbor);
    }
    return bits;
  }

  void add_edge(std::size_t u, std::size_t v) {
    if (u == v || u >= vertex_count() || v >= vertex_count()) {
      return;
    }
    adjacency_[u].push_back(v);
    adjacency_[v].push_back(u);
  }

  void sort_and_unique() {
    for (auto& neighbors : adjacency_) {
      std::sort(neighbors.begin(), neighbors.end());
      neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
    }
  }

private:
  std::vector<std::vector<std::size_t>> adjacency_;
};

}  // namespace mce