# 実験手順書: 逐次 BK から MPI まで

この文書は、このリポジトリで実験を回す順番を、修論で使える粒度まで具体化した小さな手順書です。

## ねらい

最初に逐次版を正しく動かし、その結果を見てから OpenMP と MPI に進みます。順番を逆にすると、速さや偏りの原因が分からなくなるからです。

## 手順

### 1. 逐次 BK を小さいグラフで回す

まずは `./build/maximal_clique_bk graph.txt` を実行し、極大クリークが期待どおり出ることを確認します。

確認する点:

- 出力されたクリークの個数
- 各クリークの頂点集合
- 小さい手計算例と一致するか

この段階の目的は、正しさを固定することです。ここが曖昧なまま並列化しても、性能差の解釈ができません。

### 2. 再帰木ログを取る

次に `./build/maximal_clique_bk graph.txt log.csv` を実行して、探索木の各ノードを記録します。

確認する点:

- `depth`
- `p_size`
- `child_count`
- `elapsed_us`
- `is_leaf`

必要なら `node_id` と `parent_id` から子孫合計時間を復元します。ここで、どの枝が重いかを見ます。

### 3. 解析スクリプトで偏りを見る

`python3 scripts/analyze_recursion_log.py log.csv --output-dir /tmp/bk_analysis` を実行し、深さ別の時間分布と、子孫合計時間が大きいノードを確認します。

見るべき点:

- 深さごとのノード数
- 深さごとの `elapsed_us`
- `subtree_elapsed_us` が大きいノード
- `descendant_elapsed_us` が大きいノード
- `p_size` と `child_count` の対応

この段階で、入力グラフによって探索木の偏りがどう変わるかを掴みます。

### 4. OpenMP 版を試す

`./build/maximal_clique_bk_omp graph.txt TASK_DEPTH THREADS` を回し、`task_depth` を変えながら逐次版と比較します。

比較する点:

- 実行時間
- speedup
- 偏りがどれだけ減ったか
- task 深さによる差

### 5. MPI 版を試す

`mpirun -np 4 ./build/maximal_clique_mpi_bk graph.txt DEPTH_LIMIT` を試し、浅い部分木の分配が妥当かを見ます。

比較する点:

- rank ごとの仕事量の差
- 通信量
- round-robin 配布の偏り
- 重み付き配布に置き換えたときの変化

## 小さい実験セットの作り方

最初は次の 3 種類を用意するとよいです。

- 疎なグラフ: 枝分かれが少ない入力
- 中密度のグラフ: 偏りが見えやすい入力
- 密なグラフ: 再帰木が大きくなりやすい入力

この 3 種類を使うと、「たまたまその入力で速かっただけ」を避けやすくなります。

## この順番で回すメリット

- 正しさと性能を分けて考えられる
- どこで偏りが出るかを先に観測できる
- OpenMP と MPI の比較軸が揃う
- 修論で説明しやすい実験順になる

## 先に避けるべきこと

- いきなり MPI から始める
- 小さい入力で速さだけを見る
- ログを取らずに並列化の良し悪しを判断する

これらは、原因の切り分けを難しくします。

## 関連文書

- [docs/bk_parallel_fundamentals.md](bk_parallel_fundamentals.md)
- [docs/recursion_tree_logging.md](recursion_tree_logging.md)
- [docs/openmp_taskization.md](openmp_taskization.md)
- [docs/mpi_task_distribution.md](mpi_task_distribution.md)