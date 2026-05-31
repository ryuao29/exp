#pragma once

#include "exp/graph.hpp"

#include <algorithm>
#include <cstddef>
#include <set>
#include <utility>
#include <vector>

namespace mce {

struct DegeneracyOrdering {
  std::vector<std::size_t> order;
  std::vector<std::size_t> reverse_order;
  std::vector<std::size_t> core_numbers;
  std::size_t degeneracy = 0;
};

inline DegeneracyOrdering compute_degeneracy_ordering(const Graph& graph) {
  const std::size_t vertex_count = graph.vertex_count();

  DegeneracyOrdering result;
  result.order.reserve(vertex_count);
  result.reverse_order.reserve(vertex_count);
  result.core_numbers.assign(vertex_count, 0);

  std::vector<std::size_t> degree(vertex_count, 0);
  std::set<std::pair<std::size_t, std::size_t>> active;
  for (std::size_t vertex = 0; vertex < vertex_count; ++vertex) {
    degree[vertex] = graph.degree(vertex);
    active.emplace(degree[vertex], vertex);
  }

  std::vector<bool> removed(vertex_count, false);
  while (!active.empty()) {
    const auto [current_degree, vertex] = *active.begin();
    active.erase(active.begin());

    removed[vertex] = true;
    result.order.push_back(vertex);
    result.core_numbers[vertex] = current_degree;
    result.degeneracy = std::max(result.degeneracy, current_degree);

    for (std::size_t neighbor : graph.neighbors(vertex)) {
      if (removed[neighbor]) {
        continue;
      }

      const auto it = active.find({degree[neighbor], neighbor});
      if (it == active.end()) {
        continue;
      }

      active.erase(it);
      --degree[neighbor];
      active.emplace(degree[neighbor], neighbor);
    }
  }

  result.reverse_order.assign(result.order.rbegin(), result.order.rend());
  return result;
}

}  // namespace mce