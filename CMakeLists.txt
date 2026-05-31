cmake_minimum_required(VERSION 3.16)
project(exp_maximal_clique LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(EXP_ENABLE_OPENMP "Enable OpenMP support when available" ON)

add_library(exp_common INTERFACE)
target_include_directories(exp_common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

enable_testing()

if(EXP_ENABLE_OPENMP)
  find_package(OpenMP QUIET)
endif()

find_package(MPI COMPONENTS CXX QUIET)

add_executable(maximal_clique_bfs src/maximal_clique_bfs.cpp)
target_link_libraries(maximal_clique_bfs PRIVATE exp_common)
if(OpenMP_CXX_FOUND)
  target_link_libraries(maximal_clique_bfs PRIVATE OpenMP::OpenMP_CXX)
  target_compile_definitions(maximal_clique_bfs PRIVATE USE_OPENMP)
endif()

if(MPI_CXX_FOUND)
  add_executable(maximal_clique_mpi_bfs src/maximal_clique_mpi_bfs.cpp)
  target_link_libraries(maximal_clique_mpi_bfs PRIVATE exp_common)
  target_link_libraries(maximal_clique_mpi_bfs PRIVATE MPI::MPI_CXX)
endif()

if(MPI_CXX_FOUND)
  add_executable(maximal_clique_mpi_bk src/maximal_clique_mpi_bk.cpp)
  target_link_libraries(maximal_clique_mpi_bk PRIVATE exp_common MPI::MPI_CXX)
endif()

add_executable(maximal_clique_bk src/maximal_clique_bk.cpp)
target_link_libraries(maximal_clique_bk PRIVATE exp_common)

if(OpenMP_CXX_FOUND)
  add_executable(maximal_clique_bk_omp src/maximal_clique_bk_omp.cpp)
  target_link_libraries(maximal_clique_bk_omp PRIVATE exp_common OpenMP::OpenMP_CXX)
  target_compile_definitions(maximal_clique_bk_omp PRIVATE USE_OPENMP)
endif()

add_executable(bron_kerbosch_test tests/bron_kerbosch_test.cpp)
target_link_libraries(bron_kerbosch_test PRIVATE exp_common)
add_test(NAME bron_kerbosch_test COMMAND bron_kerbosch_test)

add_executable(degeneracy_test tests/degeneracy_test.cpp)
target_link_libraries(degeneracy_test PRIVATE exp_common)
add_test(NAME degeneracy_test COMMAND degeneracy_test)

add_executable(bron_kerbosch_logging_test tests/bron_kerbosch_logging_test.cpp)
target_link_libraries(bron_kerbosch_logging_test PRIVATE exp_common)
add_test(NAME bron_kerbosch_logging_test COMMAND bron_kerbosch_logging_test)

if(OpenMP_CXX_FOUND)
  add_executable(bron_kerbosch_openmp_test tests/bron_kerbosch_openmp_test.cpp)
  target_link_libraries(bron_kerbosch_openmp_test PRIVATE exp_common OpenMP::OpenMP_CXX)
  target_compile_definitions(bron_kerbosch_openmp_test PRIVATE USE_OPENMP)
  add_test(NAME bron_kerbosch_openmp_test COMMAND bron_kerbosch_openmp_test)
endif()
