# exp

極大クリーク列挙問題の並列計算化に向けた実験用リポジトリです。

## 目的

- まず1か月で OpenMP による単一ノード並列化の基本を確認する
- その後 OpenMPI を用いて複数台 PC で幅優先探索 (BFS) ベースの探索分散を試す

## 実装内容

- `include/exp/bitset.hpp`
  - 可変長ビット集合。将来の `R/P/X` や隣接集合の交差計算に使う土台
- `include/exp/graph.hpp`
  - グラフ読込、隣接判定、近傍ビット集合の生成
- `include/exp/logger.hpp`
  - CSV ログ出力の共通部品
- `src/maximal_clique_bfs.cpp`
  - 幅優先でクリーク候補を展開する実験コード
  - OpenMP が有効な環境では frontier 展開を並列化
- `src/maximal_clique_bk.cpp`
  - pivot なしの逐次 Bron–Kerbosch 法の baseline
  - 極大クリークを 1 行 1 つで出力する
- `src/maximal_clique_bk_omp.cpp`
  - 浅い深さを OpenMP task に切る Bron–Kerbosch の並列プロトタイプ
- `include/exp/recursion_tree_logger.hpp`
  - BK の再帰木を CSV で記録するための共通部品
- `include/exp/degeneracy.hpp`
  - degeneracy ordering を計算する共通関数
- `src/maximal_clique_mpi_bfs.cpp`
  - MPI rank ごとに frontier を分担して展開
  - 各 depth ごとに root(rank 0) に集約し、次 frontier を全 rank に再配布

Week 0 のまとめは [docs/week0_foundation_modules.md](docs/week0_foundation_modules.md) に置いてあります。

逐次 BK baseline のまとめは [docs/sequential_bron_kerbosch.md](docs/sequential_bron_kerbosch.md) に置いてあります。

degeneracy ordering のまとめは [docs/degeneracy_ordering.md](docs/degeneracy_ordering.md) に置いてあります。

再帰木ログの設計と解析手順は [docs/recursion_tree_logging.md](docs/recursion_tree_logging.md) にあります。

OpenMP task 化の方針は [docs/openmp_taskization.md](docs/openmp_taskization.md) にあります。

ドキュメント更新ルールは [docs/CONTRIBUTING_DOCS.md](docs/CONTRIBUTING_DOCS.md) を参照してください。

対話的な再帰木解析ノートブックは [notebooks/recursion_analysis.ipynb](notebooks/recursion_analysis.ipynb) にあります。

ノートブックの説明は [docs/recursion_analysis_notebook.md](docs/recursion_analysis_notebook.md) にあります。

## ビルド

```bash
cmake -S . -B build
cmake --build build
```

`MPI` が見つかった場合のみ `maximal_clique_mpi_bfs` が生成されます。

OpenMP を無効化したい場合は:

```bash
cmake -S . -B build -DEXP_ENABLE_OPENMP=OFF
cmake --build build
```

## 実行

グラフ入力形式:

- 1行目: 頂点数 `N`
- 2行目以降: 辺 `u v` (0-indexed)

例:

```txt
5
0 1
0 2
1 2
2 3
3 4
```

OpenMP 実験:

```bash
./build/maximal_clique_bfs graph.txt 3 4
```

OpenMPI 実験:

```bash
mpirun -np 4 ./build/maximal_clique_mpi_bfs graph.txt 3
```

MPI-based BK prototype (if MPI is available at build time):

```bash
mpirun -np 4 ./build/maximal_clique_mpi_bk graph.txt 2
```

Note: `maximal_clique_mpi_bk` is built only when an MPI C++ implementation is found by CMake.

逐次 BK baseline:

```bash
./build/maximal_clique_bk graph.txt
```

OpenMP task 版 BK:

```bash
./build/maximal_clique_bk_omp graph.txt 2 4
```

再帰木ログ付きの逐次 BK:

```bash
./build/maximal_clique_bk graph.txt log.csv
```

テスト:

```bash
ctest --test-dir build --output-on-failure
```

## 1か月の進め方 (目安)

- Week 1: OpenMP 基礎（`parallel for`, `schedule`, `critical/reduction`）
- Week 2: クリーク探索の depth ごとの計測とボトルネック把握
- Week 3: MPI で frontier 分散・集約の実装と小規模グラフで検証
- Week 4: 複数台実行で通信オーバーヘッドを含めた性能比較
