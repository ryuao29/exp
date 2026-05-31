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

BK 法、OpenMP、MPI、並列化の前提知識をまとめた入口は [docs/bk_parallel_fundamentals.md](docs/bk_parallel_fundamentals.md) に置いてあります。

再帰木ログの設計と解析手順は [docs/recursion_tree_logging.md](docs/recursion_tree_logging.md) にあります。

OpenMP task 化の方針は [docs/openmp_taskization.md](docs/openmp_taskization.md) にあります。

MPI の分散方針は [docs/mpi_task_distribution.md](docs/mpi_task_distribution.md) にあります。

小さな実験手順書は [docs/experiment_playbook.md](docs/experiment_playbook.md) にあります。

## 学習の順番

修論の準備として読むなら、次の順番が分かりやすいです。

1. [docs/bk_parallel_fundamentals.md](docs/bk_parallel_fundamentals.md) で全体像をつかむ
2. [docs/sequential_bron_kerbosch.md](docs/sequential_bron_kerbosch.md) で逐次 BK の流れを理解する
3. [docs/degeneracy_ordering.md](docs/degeneracy_ordering.md) で探索順の前処理を理解する
4. [docs/recursion_tree_logging.md](docs/recursion_tree_logging.md) で観測方法を理解する
5. [docs/openmp_taskization.md](docs/openmp_taskization.md) で単一ノード並列の考え方を理解する
6. [docs/mpi_task_distribution.md](docs/mpi_task_distribution.md) で分散実行の考え方を理解する

この順で読むと、「何を観測して」「何を比較して」「どこが研究ギャップになりそうか」を追いやすくなります。

実際の実験を回すときは、[docs/experiment_playbook.md](docs/experiment_playbook.md) に従って、逐次版 -> ログ解析 -> OpenMP -> MPI の順に進めると迷いにくいです。

## 実験の進め方

まずは逐次版を小さいグラフで動かして、結果が正しいかを固めます。ここで見るのは、出力された極大クリークの個数と内容です。正しさが曖昧なまま並列化すると、速さの差がアルゴリズム差なのか実装差なのか分からなくなります。

次に再帰木ログを取って、どの深さやどの部分木が重いかを確認します。特に `depth`、`p_size`、`child_count`、`elapsed_us` を見て、探索木がどこで偏るかを把握します。`node_id` と `parent_id` があるので、子孫合計時間を後から計算して「重い枝」を追うこともできます。

その後で OpenMP 版を試し、`task_depth` を変えながら逐次版と比較します。ここでは、単純な実行時間だけでなく、ログから見た偏りが減っているかも一緒に見ます。速くなったとしても、偏りが残るなら task の切り方を変える余地があります。

OpenMP で改善の兆しが見えたら、MPI 版に進みます。まずは shallow subtree をどう切るかを確認し、round-robin 配布と重み付き配布を比べます。MPI では通信コストが効くので、単に分けるだけでなく、どの深さで切ると通信量と負荷均衡のバランスがよいかを見ます。

この順番は、以下の確認を崩さないためのものです。

- 逐次で正しいことを先に固定する
- ログで偏りの形を観測する
- OpenMP で単一ノードの分割条件を試す
- MPI で分散したときの通信と負荷均衡を比較する

この流れで進めると、後から「何が効いたのか」を説明しやすくなります。

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
