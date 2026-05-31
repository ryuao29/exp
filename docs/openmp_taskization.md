# OpenMP task 化の説明

この文書は、Bron–Kerbosch の浅い深さ task 化をどう入れたか、何を保証していて、どこまでを並列化対象にしているかをまとめたものです。

## 目的

最初の OpenMP 版は、探索木の浅い部分を task に切って、単一ノード上で負荷を分散できるかを確認するためのものです。

この段階の目標は「速い最終版」ではなく、「後で MPI にもつながる分割単位を見つけること」です。

## 実装の考え方

- 既存の逐次 BK baseline を壊さず、別の並列エントリポイントを追加した
- task は深さ制限つきで生成する
- 各 task はその時点の `P` と `X` のコピーを持つので、兄弟探索と干渉しない
- クリク報告は callback を `critical` で保護している

対象コード: `include/exp/bron_kerbosch.hpp`

CLI: `src/maximal_clique_bk_omp.cpp`

## 重要な制約

- この初版では再帰木 CSV ログは並列版に付けていない
- task の粒度が小さすぎるとオーバーヘッドが勝つので、`task_depth` を 1 から順に試す
- まずは正しさを優先し、性能最適化は後で行う

## 使い方

```bash
./build/maximal_clique_bk_omp graph.txt 2 4
```

- 2 番目の引数: task を生成する最大深さ
- 3 番目の引数: OpenMP スレッド数

## これからのつながり

この task 化の結果は、ノートブック `notebooks/recursion_analysis.ipynb` で見た偏りと合わせて評価する。

そのあとで、MPI に流す浅い部分木の切り方を決める。

## 更新ルール

- task の深さ条件や callback の扱いを変えたら、この文書を更新する
- CLI 引数を増やしたら `README.md` も更新する
- 正しさを変える修正を入れたら、対応するテストを増やす
