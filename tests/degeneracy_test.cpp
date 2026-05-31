#include "exp/degeneracy.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <vector>

int main() {
  mce::Graph graph(5);
  graph.add_edge(0, 1);
  graph.add_edge(0, 2);
  graph.add_edge(1, 2);
  graph.add_edge(2, 3);
  graph.add_edge(3, 4);
  graph.sort_and_unique();

  const auto ordering = mce::compute_degeneracy_ordering(graph);

  assert(ordering.order.size() == 5);
  assert(ordering.reverse_order.size() == 5);
  assert(ordering.core_numbers.size() == 5);
  assert(ordering.degeneracy == 2);

  std::vector<bool> seen(5, false);
  for (std::size_t vertex : ordering.order) {
    assert(vertex < 5);
    assert(!seen[vertex]);
    seen[vertex] = true;
  }

  for (bool flag : seen) {
    assert(flag);
  }

  for (std::size_t index = 0; index < ordering.order.size(); ++index) {
    const std::size_t vertex = ordering.order[index];
    std::size_t later_neighbors = 0;
    for (std::size_t neighbor : graph.neighbors(vertex)) {
      const auto it = std::find(ordering.order.begin() + static_cast<std::ptrdiff_t>(index), ordering.order.end(), neighbor);
      if (it != ordering.order.end()) {
        ++later_neighbors;
      }
    }
    assert(later_neighbors <= ordering.degeneracy);
  }

  for (std::size_t vertex = 0; vertex < 5; ++vertex) {
    if (vertex == 4) {
      assert(ordering.core_numbers[vertex] == 1);
    }
  }

  return 0;
}