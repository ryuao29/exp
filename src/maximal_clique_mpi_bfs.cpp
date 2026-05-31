#include <mpi.h>

#include <iostream>
#include <algorithm>
#include <string>
#include <vector>

#include "exp/graph.hpp"

using mce::Graph;

std::vector<int> flatten(const std::vector<std::vector<int>>& frontier, int clique_width) {
  std::vector<int> out;
  out.reserve(frontier.size() * static_cast<size_t>(clique_width));
  for (const auto& clique : frontier) {
    auto tmp = clique;
    tmp.resize(static_cast<size_t>(clique_width), -1);
    out.insert(out.end(), tmp.begin(), tmp.end());
  }
  return out;
}

std::vector<std::vector<int>> unflatten(const std::vector<int>& data, int clique_width) {
  std::vector<std::vector<int>> out;
  if (clique_width <= 0) {
    return out;
  }
  const size_t rows = data.size() / static_cast<size_t>(clique_width);
  out.reserve(rows);
  for (size_t i = 0; i < rows; ++i) {
    std::vector<int> clique;
    clique.reserve(static_cast<size_t>(clique_width));
    for (int j = 0; j < clique_width; ++j) {
      int x = data[i * static_cast<size_t>(clique_width) + static_cast<size_t>(j)];
      if (x >= 0) {
        clique.push_back(x);
      }
    }
    out.push_back(std::move(clique));
  }
  return out;
}

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  int rank = 0;
  int world = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world);

  if (argc < 3) {
    if (rank == 0) {
      std::cerr << "usage: " << argv[0] << " <graph.txt> <depth>\n";
    }
    MPI_Finalize();
    return 1;
  }

  const auto graph = Graph::load_from_file(argv[1]);
  const int depth_limit = std::max(0, std::stoi(argv[2]));

  std::vector<std::vector<int>> frontier(1);

  for (int depth = 0; depth < depth_limit; ++depth) {
    const int clique_width = depth;
    std::vector<std::vector<int>> local_next;

    for (size_t i = static_cast<size_t>(rank); i < frontier.size(); i += static_cast<size_t>(world)) {
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

    auto send_data = flatten(local_next, clique_width + 1);
    int send_count = static_cast<int>(send_data.size());

    std::vector<int> counts(static_cast<size_t>(world));
    MPI_Gather(&send_count, 1, MPI_INT, counts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    std::vector<int> displs(static_cast<size_t>(world), 0);
    int recv_total = 0;
    if (rank == 0) {
      for (int i = 0; i < world; ++i) {
        displs[static_cast<size_t>(i)] = recv_total;
        recv_total += counts[static_cast<size_t>(i)];
      }
    }

    std::vector<int> recv_data(static_cast<size_t>(std::max(0, recv_total)));
    MPI_Gatherv(send_data.data(), send_count, MPI_INT, recv_data.data(), counts.data(), displs.data(), MPI_INT, 0,
                MPI_COMM_WORLD);

    if (rank == 0) {
      frontier = unflatten(recv_data, clique_width + 1);
      std::cout << "depth=" << (depth + 1) << " frontier=" << frontier.size() << '\n';
    }

    int rows = (rank == 0) ? static_cast<int>(frontier.size()) : 0;
    MPI_Bcast(&rows, 1, MPI_INT, 0, MPI_COMM_WORLD);

    std::vector<int> flat_frontier;
    if (rank == 0) {
      flat_frontier = flatten(frontier, clique_width + 1);
    } else {
      flat_frontier.resize(static_cast<size_t>(rows) * static_cast<size_t>(clique_width + 1), -1);
    }

    MPI_Bcast(flat_frontier.data(), static_cast<int>(flat_frontier.size()), MPI_INT, 0, MPI_COMM_WORLD);
    frontier = unflatten(flat_frontier, clique_width + 1);

    if (rows == 0) {
      break;
    }
  }

  MPI_Finalize();
  return 0;
}
