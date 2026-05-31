#include "exp/graph.hpp"
#include "exp/bitset.hpp"

#include <mpi.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using mce::Bitset;
using mce::Clique;
using mce::Graph;

// Subproblem representation
struct Subproblem {
  Clique current;
  std::vector<uint64_t> p_words;
  std::vector<uint64_t> x_words;
  std::size_t bit_count = 0;
};

static void generate_shallow_subproblems(const std::vector<Bitset>& neighbors,
                                        Bitset p,
                                        Bitset x,
                                        Clique& current,
                                        int depth_limit,
                                        int depth,
                                        std::vector<Subproblem>& out) {
  if (depth >= depth_limit) {
    Subproblem s;
    s.current = current;
    s.p_words = p.words();
    s.x_words = x.words();
    s.bit_count = p.size();
    out.push_back(std::move(s));
    return;
  }

  const auto candidates = p.indices();
  if (candidates.empty()) {
    Subproblem s;
    s.current = current;
    s.p_words = p.words();
    s.x_words = x.words();
    s.bit_count = p.size();
    out.push_back(std::move(s));
    return;
  }

  for (auto v : candidates) {
    current.push_back(v);
    Bitset next_p = p & neighbors[v];
    Bitset next_x = x & neighbors[v];
    generate_shallow_subproblems(neighbors, next_p, next_x, current, depth_limit, depth + 1, out);
    current.pop_back();
    p.reset(v);
    x.set(v);
  }
}

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);
  int rank = 0, nprocs = 1;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  if (argc < 3) {
    if (rank == 0) {
      std::cerr << "usage: " << argv[0] << " <graph.txt> <depth_limit>\n";
    }
    MPI_Finalize();
    return 1;
  }

  const std::string graph_path = argv[1];
  const int depth_limit = std::max(0, std::stoi(argv[2]));

  // each rank loads graph locally
  const auto graph = Graph::load_from_file(graph_path);
  const std::size_t n = graph.vertex_count();
  std::vector<Bitset> neighbors; neighbors.reserve(n);
  for (std::size_t i = 0; i < n; ++i) neighbors.push_back(graph.neighbor_bitset(i));

  std::vector<Subproblem> assigned;

  if (rank == 0) {
    // generate shallow subproblems
    Bitset p(n); for (std::size_t i = 0; i < n; ++i) p.set(i);
    Bitset x(n);
    Clique cur;
    std::vector<Subproblem> subs;
    generate_shallow_subproblems(neighbors, p, x, cur, depth_limit, 0, subs);

    // distribute subs in round-robin
    for (std::size_t i = 0; i < subs.size(); ++i) {
      int target = static_cast<int>(i % nprocs);
      if (target == 0) {
        assigned.push_back(std::move(subs[i]));
        continue;
      }
      // send to target: serialize sizes then words and current
      uint64_t bit_count = subs[i].bit_count;
      uint64_t p_words_n = subs[i].p_words.size();
      uint64_t x_words_n = subs[i].x_words.size();
      uint64_t current_n = subs[i].current.size();
      MPI_Send(&bit_count, 1, MPI_UINT64_T, target, 1, MPI_COMM_WORLD);
      MPI_Send(&p_words_n, 1, MPI_UINT64_T, target, 1, MPI_COMM_WORLD);
      if (p_words_n) MPI_Send(subs[i].p_words.data(), p_words_n, MPI_UINT64_T, target, 1, MPI_COMM_WORLD);
      MPI_Send(&x_words_n, 1, MPI_UINT64_T, target, 1, MPI_COMM_WORLD);
      if (x_words_n) MPI_Send(subs[i].x_words.data(), x_words_n, MPI_UINT64_T, target, 1, MPI_COMM_WORLD);
      MPI_Send(&current_n, 1, MPI_UINT64_T, target, 1, MPI_COMM_WORLD);
      if (current_n) MPI_Send(subs[i].current.data(), current_n, MPI_UINT64_T, target, 1, MPI_COMM_WORLD);
    }
    // send termination signal: bit_count = 0
    uint64_t term = 0;
    for (int dest = 1; dest < nprocs; ++dest) MPI_Send(&term, 1, MPI_UINT64_T, dest, 2, MPI_COMM_WORLD);
  } else {
    // receive assigned subs until termination
    while (true) {
      uint64_t bit_count = 0;
      MPI_Recv(&bit_count, 1, MPI_UINT64_T, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      if (bit_count == 0) break; // termination
      uint64_t p_words_n = 0; MPI_Recv(&p_words_n, 1, MPI_UINT64_T, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      Subproblem s;
      s.bit_count = bit_count;
      if (p_words_n) {
        s.p_words.resize(p_words_n);
        MPI_Recv(s.p_words.data(), p_words_n, MPI_UINT64_T, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
      uint64_t x_words_n = 0; MPI_Recv(&x_words_n, 1, MPI_UINT64_T, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      if (x_words_n) {
        s.x_words.resize(x_words_n);
        MPI_Recv(s.x_words.data(), x_words_n, MPI_UINT64_T, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
      uint64_t current_n = 0; MPI_Recv(&current_n, 1, MPI_UINT64_T, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      if (current_n) {
        s.current.resize(current_n);
        MPI_Recv(s.current.data(), current_n, MPI_UINT64_T, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
      assigned.push_back(std::move(s));
    }
  }

  // Each rank now processes its assigned subproblems locally
  std::size_t local_count = 0;
  for (auto &s : assigned) {
    Bitset p(s.bit_count), x(s.bit_count);
    p.set_words(s.bit_count, s.p_words);
    x.set_words(s.bit_count, s.x_words);
    Clique current = s.current;

    // local enumeration using task-parallel entrypoint with small task depth
    mce::enumerate_maximal_cliques_no_pivot_task_parallel(graph, [&](const Clique& clique){
      // for distributed run, print with rank prefix
      std::cout << "rank=" << rank << " ";
      for (size_t i=0;i<clique.size();++i) { if (i) std::cout << ' '; std::cout << clique[i]; }
      std::cout << '\n';
      ++local_count;
    }, 1);
  }

  MPI_Barrier(MPI_COMM_WORLD);
  if (rank == 0) std::cerr << "MPI enumeration done" << std::endl;

  MPI_Finalize();
  return 0;
}
