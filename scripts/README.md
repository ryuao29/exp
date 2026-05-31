# Recursion log analysis scripts

This folder contains a small utility to analyze the CSV logs produced by `maximal_clique_bk` when run with a log path.

Usage:

```bash
python3 scripts/analyze_recursion_log.py /path/to/log.csv --output-dir /tmp/bk_analysis
```

Outputs:

- `/tmp/bk_analysis/depth_summary.csv` : per-depth node counts and elapsed_us totals
- `/tmp/bk_analysis/top_nodes.csv` : top nodes by subtree elapsed_us

The script will attempt to produce PNG plots if `matplotlib` is installed.

If you want plots, install `matplotlib`:

```bash
python3 -m pip install matplotlib
```

