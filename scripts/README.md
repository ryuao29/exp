# Recursion log analysis scripts

This folder contains a small utility to analyze the CSV logs produced by `maximal_clique_bk` when run with a log path.

Usage:

```bash
python3 scripts/analyze_recursion_log.py log.csv
python3 scripts/analyze_recursion_log.py log.csv --output-dir analyze
```

Recommended execution location:

- Run from the repository root so outputs stay inside the project.
- With the default setting, results are written to `analyze/` under the current working directory.

Outputs:

- `analyze/depth_summary.csv`
	- One row per recursion depth.
	- Contains node count and total `elapsed_us` at each depth.
	- Useful for seeing where the search tree is wide or expensive.
- `analyze/depth_subtree_summary.csv`
	- Per-depth aggregate of subtree cost (`subtree_elapsed_us`) and descendant cost.
	- Useful for understanding how much work hangs below each depth.
- `analyze/top_nodes.csv`
	- Nodes sorted by subtree elapsed time (top heavy nodes).
	- Useful for identifying bottleneck subtrees.
- `analyze/node_subtree_summary.csv`
	- Per-node subtree totals and ratios (`self_time_share`, subtree node counts).
	- Useful for detailed drill-down by node.
- `analyze/depth_distribution.png` (if matplotlib is available)
	- Combined depth vs node-count / elapsed plot.
- `analyze/top_subtree_elapsed.png` (if matplotlib is available)
	- Bar chart for top subtree costs.
- `analyze/depth_subtree_elapsed.png` (if matplotlib is available)
	- Depth-level subtree elapsed summary plot.

Console summary meaning:

- `nodes`: number of recursion nodes loaded from CSV.
- `total elapsed_us`: sum of per-node `elapsed_us` values.
- `... written to ...`: output artifact path for each generated table/plot.

The script will attempt to produce PNG plots if `matplotlib` is installed.

If you want plots, install `matplotlib`:

```bash
python3 -m pip install matplotlib
```

