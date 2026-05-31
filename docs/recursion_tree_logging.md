# 再帰木ログ（Recursion Tree Logging）説明書

この文書は、Bron–Kerbosch 実装で出力される再帰木 CSV ログの設計、用途、解析方法、及びログを追加・改訂する際のルールをまとめたものです。

## 目的

- BK の各再帰ノードで「どれだけ時間を使っているか」「候補集合の大きさがどうなっているか」を記録し、探索木の偏り（負荷分布）を定量的に示す。
- ログを用いて、どの深さやどの起点が重いかを見つけ、OpenMP や MPI での分割基準（深さ閾値や `|P|` 閾値）を決める。

## 出力フォーマット

CSV ヘッダ（列順）:

- `node_id` : ノードの一意 ID（0 から連番）
- `parent_id` : 親ノードの ID（ルートは `-1`）
- `depth` : ルートを 0 とした深さ
- `clique_size` : 現在の `R`（完成しているクリーク）のサイズ
- `p_size` : `P` のサイズ
- `x_size` : `X` のサイズ
- `candidate_count` : このノードで列挙対象となった候補数（`P` の要素を順に試す回数）
- `child_count` : 実際に生成された子ノード数
- `elapsed_us` : このノードの処理にかかったマイクロ秒単位の実時間
- `is_leaf` : 葉ノードなら `1`、それ以外は `0`

実装参照: `include/exp/recursion_tree_logger.hpp`

## 使い方（解析の流れ）

1. BK 実行時に第2引数で CSV パスを指定する: `./build/maximal_clique_bk graph.txt log.csv`
2. 得られた CSV を Python / pandas で読み込み、深さ毎の `elapsed_us` 合計や `p_size` 分布を集計する。
3. 「重いサブツリー」を見つける: ノードの `elapsed_us` が大きいか、子孫の合計 `elapsed_us` が大きい部分木を抽出する。
4. OpenMP タスク化の閾値候補を選ぶ: 例えば `depth <= D && p_size >= Pmin` のような経験則を CSV 分布から決定する。

簡単な解析スニペット（Python, pandas）例:

```python
import pandas as pd
df = pd.read_csv('log.csv')
by_depth = df.groupby('depth').agg({'elapsed_us':'sum','node_id':'count'})
print(by_depth)
```

## ログの精度とオーバーヘッド

- ログは比較的軽量に保つ設計ですが、高頻度でファイル I/O が発生すると実測時間に影響します。実験での比較時はログの有無によるオーバーヘッドを事前に確認してください。
- `RecursionTreeCsvLogger` は `enabled` フラグで無効化できます。計測時はログ無し／有りで差を比較してオーバーヘッドを評価してください。

## ドキュメント更新ルール（このリポジトリの運用規約）

1. コードに新しいログ項目（CSV 列）を追加する場合は、必ずこの `docs/recursion_tree_logging.md` を更新すること。
2. ログ項目の型や単位（例えば `elapsed_us` がマイクロ秒であること）を変更する場合は、後方互換性を保つか、`version` 列を追加して変更履歴を残すこと。
3. ログのフォーマット変更に合わせて、`tests/bron_kerbosch_logging_test.cpp` を更新し、自動テストでヘッダの存在と最小限の行があることを検査すること。
4. ドキュメント更新時はコミットメッセージに `docs:` プレフィックスを付けること（例: `docs: update recursion tree logging format`）。

## 追加の解析提案

- 各ノードの子孫合計 `elapsed_us` を計算し、重心の深さや子孫分布を可視化する。
- ノードを重み付きでクラスタリングし、類似した「重いパターン」を抽出する。

---

最後に、このドキュメントは他の説明書と同様に `README.md` の「ドキュメント」セクションにリンクを追加しておきます。
