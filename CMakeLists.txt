cmake_minimum_required(VERSION 3.16)
project(exp_maximal_clique LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenMP)
find_package(MPI COMPONENTS CXX)

add_executable(maximal_clique_bfs src/maximal_clique_bfs.cpp)
if(OpenMP_CXX_FOUND)
  target_link_libraries(maximal_clique_bfs PRIVATE OpenMP::OpenMP_CXX)
  target_compile_definitions(maximal_clique_bfs PRIVATE USE_OPENMP)
endif()

if(MPI_CXX_FOUND)
  add_executable(maximal_clique_mpi_bfs src/maximal_clique_mpi_bfs.cpp)
  target_link_libraries(maximal_clique_mpi_bfs PRIVATE MPI::MPI_CXX)
endif()
