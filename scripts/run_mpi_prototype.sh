#!/usr/bin/env bash
set -euo pipefail

# Run MPI prototype with a generated random graph, or print instructions if MPI not available.
# Usage: ./scripts/run_mpi_prototype.sh [--nodes N] [--prob P] [--depth D] [--procs R]

N=50
P=0.1
D=2
R=4

while [[ $# -gt 0 ]]; do
  case "$1" in
    --nodes) N="$2"; shift 2;;
    --prob) P="$2"; shift 2;;
    --depth) D="$2"; shift 2;;
    --procs) R="$2"; shift 2;;
    -h|--help) echo "Usage: $0 [--nodes N] [--prob P] [--depth D] [--procs R]"; exit 0;;
    *) echo "Unknown arg: $1"; exit 1;;
  esac
done

GRAPH=/tmp/mc_graph.txt
OUT=/tmp/mc_mpi_out.txt

echo "generating random graph: nodes=${N}, prob=${P} -> ${GRAPH}"
python3 - <<PY > ${GRAPH}
import random
N = ${N}
P = float('${P}')
print(N)
for u in range(N):
    for v in range(u+1, N):
        if random.random() < P:
            print(u, v)
PY

BIN=build/maximal_clique_mpi_bk
if [[ -x "${BIN}" && $(command -v mpirun || true) ]]; then
  echo "running mpirun with ${R} procs, depth ${D}"
  mpirun -np ${R} ${BIN} ${GRAPH} ${D} | tee ${OUT}
  echo "output saved to ${OUT}"
else
  echo "MPI binary or mpirun not found. To run this prototype on an MPI-enabled host:" >&2
  echo "  1) Ensure MPI C++ libraries are installed (OpenMPI or MPICH)." >&2
  echo "  2) Reconfigure and build:" >&2
  echo "       cmake -S . -B build" >&2
  echo "       cmake --build build --target maximal_clique_mpi_bk" >&2
  echo "  3) Run with mpirun on the target host:" >&2
  echo "       mpirun -np ${R} ./build/maximal_clique_mpi_bk ${GRAPH} ${D}" >&2
  exit 2
fi
