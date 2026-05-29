# exp

極大クリーク列挙問題の並列計算化に向けた実験用リポジトリです。

## 目的

- まず1か月で OpenMP による単一ノード並列化の基本を確認する
- その後 OpenMPI を用いて複数台 PC で幅優先探索 (BFS) ベースの探索分散を試す

## 実装内容

- `src/maximal_clique_bfs.cpp`
  - 幅優先でクリーク候補を展開する実験コード
  - OpenMP が有効な環境では frontier 展開を並列化
- `src/maximal_clique_mpi_bfs.cpp`
  - MPI rank ごとに frontier を分担して展開
  - 各 depth ごとに root(rank 0) に集約し、次 frontier を全 rank に再配布

## ビルド

```bash
cmake -S . -B build
cmake --build build
```

`MPI` が見つかった場合のみ `maximal_clique_mpi_bfs` が生成されます。

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

## 1か月の進め方 (目安)

- Week 1: OpenMP 基礎（`parallel for`, `schedule`, `critical/reduction`）
- Week 2: クリーク探索の depth ごとの計測とボトルネック把握
- Week 3: MPI で frontier 分散・集約の実装と小規模グラフで検証
- Week 4: 複数台実行で通信オーバーヘッドを含めた性能比較
