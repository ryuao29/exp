#pragma once

#include "exp/bitset.hpp"
#include "exp/graph.hpp"
#include "exp/recursion_tree_logger.hpp"

#include <chrono>
#include <cstddef>
#include <utility>
#include <vector>

namespace mce {

using Clique = std::vector<std::size_t>;

template <typename Callback>
void bron_kerbosch_no_pivot_recursive(const std::vector<Bitset>& neighbors,
                                      Bitset& p,
                                      Bitset& x,
                                      Clique& current,
                                      Callback& callback,
                                      RecursionTreeCsvLogger* logger = nullptr,
                                      std::size_t parent_id = kNoParentNode,
                                      std::size_t depth = 0) {
  const auto start_time = std::chrono::steady_clock::now();
  const std::size_t node_id = logger ? logger->next_node_id() : 0;
  const std::vector<std::size_t> candidates = p.indices();
  const std::size_t p_size = p.count();
  const std::size_t x_size = x.count();
  std::size_t child_count = 0;
  const bool is_leaf = p.none() && x.none();

  if (is_leaf) {
    callback(current);
  } else {
    for (const std::size_t vertex : candidates) {
      current.push_back(vertex);

      Bitset next_p = p & neighbors[vertex];
      Bitset next_x = x & neighbors[vertex];

      ++child_count;
      bron_kerbosch_no_pivot_recursive(neighbors,
                                       next_p,
                                       next_x,
                                       current,
                                       callback,
                                       logger,
                                       node_id,
                                       depth + 1);

      current.pop_back();
      p.reset(vertex);
      x.set(vertex);
    }
  }

  if (logger) {
    const auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
                                std::chrono::steady_clock::now() - start_time)
                                .count();
    logger->write_node({node_id,
                        parent_id,
                        depth,
                        current.size(),
                        p_size,
                        x_size,
                        candidates.size(),
                        child_count,
                        static_cast<std::uint64_t>(elapsed_us),
                        static_cast<std::uint8_t>(is_leaf)});
  }
}

template <typename Callback>
void bron_kerbosch_no_pivot_task_recursive(const std::vector<Bitset>& neighbors,
                                           Bitset p,
                                           Bitset x,
                                           Clique current,
                                           Callback& callback,
                                           int task_depth_limit,
                                           std::size_t depth = 0) {
  const std::vector<std::size_t> candidates = p.indices();
  const bool is_leaf = p.none() && x.none();

  if (is_leaf) {
#ifdef USE_OPENMP
#pragma omp critical(mce_bk_callback)
#endif
    callback(current);
    return;
  }

  const bool spawn_tasks = task_depth_limit > 0 && depth < static_cast<std::size_t>(task_depth_limit) &&
                           candidates.size() > 1;

#ifdef USE_OPENMP
  if (spawn_tasks) {
#pragma omp taskgroup
    {
      for (const std::size_t vertex : candidates) {
        Clique next_current = current;
        next_current.push_back(vertex);
        Bitset next_p = p & neighbors[vertex];
        Bitset next_x = x & neighbors[vertex];
#pragma omp task default(none) firstprivate(next_current, next_p, next_x, depth) shared(callback, neighbors, task_depth_limit)
        {
          bron_kerbosch_no_pivot_task_recursive(neighbors,
                                                std::move(next_p),
                                                std::move(next_x),
                                                std::move(next_current),
                                                callback,
                                                task_depth_limit,
                                                depth + 1);
        }
        p.reset(vertex);
        x.set(vertex);
      }
    }
    return;
  }
#endif

  for (const std::size_t vertex : candidates) {
    Clique next_current = current;
    next_current.push_back(vertex);
    Bitset next_p = p & neighbors[vertex];
    Bitset next_x = x & neighbors[vertex];
    bron_kerbosch_no_pivot_task_recursive(neighbors,
                                          std::move(next_p),
                                          std::move(next_x),
                                          std::move(next_current),
                                          callback,
                                          task_depth_limit,
                                          depth + 1);
    p.reset(vertex);
    x.set(vertex);
  }
}

template <typename Callback>
void enumerate_maximal_cliques_no_pivot(const Graph& graph,
                                        Callback&& callback,
                                        RecursionTreeCsvLogger* logger = nullptr) {
  const std::size_t n = graph.vertex_count();
  std::vector<Bitset> neighbors;
  neighbors.reserve(n);
  for (std::size_t vertex = 0; vertex < n; ++vertex) {
    neighbors.push_back(graph.neighbor_bitset(vertex));
  }

  Bitset p(n);
  for (std::size_t vertex = 0; vertex < n; ++vertex) {
    p.set(vertex);
  }

  Bitset x(n);
  Clique current;
  bron_kerbosch_no_pivot_recursive(neighbors, p, x, current, callback, logger);
}

template <typename Callback>
void enumerate_maximal_cliques_no_pivot_task_parallel(const Graph& graph,
                                                       Callback&& callback,
                                                       int task_depth_limit = 1) {
  if (task_depth_limit <= 0) {
    enumerate_maximal_cliques_no_pivot(graph, std::forward<Callback>(callback));
    return;
  }

  const std::size_t n = graph.vertex_count();
  std::vector<Bitset> neighbors;
  neighbors.reserve(n);
  for (std::size_t vertex = 0; vertex < n; ++vertex) {
    neighbors.push_back(graph.neighbor_bitset(vertex));
  }

  Bitset p(n);
  for (std::size_t vertex = 0; vertex < n; ++vertex) {
    p.set(vertex);
  }

  Bitset x(n);
  Clique current;
  auto& callback_ref = callback;

#ifdef USE_OPENMP
#pragma omp parallel
  {
#pragma omp single nowait
    {
      bron_kerbosch_no_pivot_task_recursive(neighbors, p, x, current, callback_ref, task_depth_limit);
    }
  }
#else
  bron_kerbosch_no_pivot_task_recursive(neighbors, p, x, current, callback_ref, task_depth_limit);
#endif
}

}  // namespace mce