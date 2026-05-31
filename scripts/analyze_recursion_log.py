#!/usr/bin/env python3
"""
Simple analyzer for recursion tree CSV logs produced by maximal_clique_bk.

Usage:
  python3 scripts/analyze_recursion_log.py /path/to/log.csv [--output-dir DIR]

Outputs summary to stdout and writes CSVs `depth_summary.csv` and `top_nodes.csv` into output dir.
"""
import csv
import os
import sys
from collections import defaultdict, deque


def read_log(path):
    nodes = {}
    children = defaultdict(list)
    with open(path, newline='') as f:
        reader = csv.DictReader(f)
        for row in reader:
            nid = int(row['node_id'])
            # normalize parent: treat '-1' or very large sentinel as -1
            raw_parent = row['parent_id']
            try:
                parent_val = int(raw_parent)
            except Exception:
                parent_val = -1
            if raw_parent == '-1' or parent_val >= (1 << 60):
                parent = -1
            else:
                parent = parent_val
            depth = int(row.get('depth', 0))
            elapsed = int(row.get('elapsed_us', 0))
            p_size = int(row.get('p_size', 0))
            candidate_count = int(row.get('candidate_count', 0))
            child_count = int(row.get('child_count', 0))
            is_leaf = int(row.get('is_leaf', 0))
            nodes[nid] = {
                'node_id': nid,
                'parent_id': parent,
                'depth': depth,
                'elapsed_us': elapsed,
                'p_size': p_size,
                'candidate_count': candidate_count,
                'child_count': child_count,
                'is_leaf': is_leaf,
                'subtree_elapsed': None,
            }
            if parent != -1:
                children[parent].append(nid)
    return nodes, children


def compute_subtree_elapsed(nodes, children):
    # Post-order traversal from roots
    visited = set()
    order = []
    # roots: parent == -1 or parent not present in nodes (robust against malformed parent ids)
    roots = [nid for nid, n in nodes.items() if n['parent_id'] == -1 or n['parent_id'] not in nodes]

    stack = []
    for r in roots:
        stack.append((r, False))
        while stack:
            nid, visited_flag = stack.pop()
            if visited_flag:
                order.append(nid)
                continue
            stack.append((nid, True))
            for c in children.get(nid, []):
                stack.append((c, False))

    for nid in order:
        total = nodes[nid]['elapsed_us']
        for c in children.get(nid, []):
            child_sub = nodes[c].get('subtree_elapsed')
            if child_sub is None:
                child_sub = nodes[c]['elapsed_us']
            total += child_sub
        nodes[nid]['subtree_elapsed'] = total


def summarize(nodes):
    total_nodes = len(nodes)
    total_elapsed = sum(n['elapsed_us'] for n in nodes.values())
    by_depth = defaultdict(lambda: {'nodes':0,'elapsed':0})
    for n in nodes.values():
        d = n['depth']
        by_depth[d]['nodes'] += 1
        by_depth[d]['elapsed'] += n['elapsed_us']
    return total_nodes, total_elapsed, by_depth


def write_depth_csv(by_depth, outpath):
    rows = sorted(by_depth.items())
    with open(outpath, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['depth','node_count','elapsed_us'])
        for depth, info in rows:
            w.writerow([depth, info['nodes'], info['elapsed']])


def write_top_nodes(nodes, outpath, key='subtree_elapsed', topk=20):
    vals = sorted(nodes.values(), key=lambda x: (x.get(key) if x.get(key) is not None else x.get('elapsed_us',0)), reverse=True)[:topk]
    with open(outpath, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['node_id','parent_id','depth','elapsed_us','subtree_elapsed','p_size','candidate_count','child_count','is_leaf'])
        for n in vals:
            w.writerow([n['node_id'], n['parent_id'], n['depth'], n['elapsed_us'], n.get('subtree_elapsed',0), n['p_size'], n['candidate_count'], n['child_count'], n['is_leaf']])


def main():
    if len(sys.argv) < 2:
        print('usage: analyze_recursion_log.py /path/to/log.csv [--output-dir DIR]')
        sys.exit(2)
    path = sys.argv[1]
    outdir = '/tmp/bk_analysis'
    if '--output-dir' in sys.argv:
        i = sys.argv.index('--output-dir')
        if i+1 < len(sys.argv):
            outdir = sys.argv[i+1]

    if not os.path.exists(path):
        print('error: log file not found:', path)
        sys.exit(3)

    nodes, children = read_log(path)
    if not nodes:
        print('no data in log')
        sys.exit(0)

    compute_subtree_elapsed(nodes, children)
    total_nodes, total_elapsed, by_depth = summarize(nodes)

    os.makedirs(outdir, exist_ok=True)
    depth_csv = os.path.join(outdir, 'depth_summary.csv')
    top_csv = os.path.join(outdir, 'top_nodes.csv')
    write_depth_csv(by_depth, depth_csv)
    write_top_nodes(nodes, top_csv, key='subtree_elapsed', topk=50)

    print('nodes:', total_nodes)
    print('total elapsed_us:', total_elapsed)
    print('depth summary written to', depth_csv)
    print('top nodes written to', top_csv)

    # Try to produce PNG plots if matplotlib is available
    try:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt

        # depth distribution plot
        depths = sorted(by_depth.items())
        xs = [d for d, _ in depths]
        node_counts = [info['nodes'] for _, info in depths]
        elapsed_sums = [info['elapsed'] for _, info in depths]

        fig, ax1 = plt.subplots(figsize=(8,4))
        ax1.bar(xs, node_counts, color='C0', alpha=0.6, label='node_count')
        ax1.set_xlabel('depth')
        ax1.set_ylabel('node count', color='C0')
        ax2 = ax1.twinx()
        ax2.plot(xs, elapsed_sums, color='C1', marker='o', label='elapsed_us')
        ax2.set_ylabel('elapsed_us', color='C1')
        plt.title('Depth distribution: node count and elapsed_us')
        fig.tight_layout()
        depth_png = os.path.join(outdir, 'depth_distribution.png')
        fig.savefig(depth_png)
        plt.close(fig)

        # top nodes by subtree elapsed
        top_nodes = sorted(nodes.values(), key=lambda x: (x.get('subtree_elapsed') if x.get('subtree_elapsed') is not None else x.get('elapsed_us',0)), reverse=True)[:50]
        labels = [str(n['node_id']) for n in top_nodes]
        vals = [n.get('subtree_elapsed') if n.get('subtree_elapsed') is not None else n.get('elapsed_us',0) for n in top_nodes]
        fig2, ax = plt.subplots(figsize=(10,6))
        ax.bar(labels, vals)
        ax.set_xlabel('node_id')
        ax.set_ylabel('subtree_elapsed_us')
        ax.set_title('Top nodes by subtree elapsed_us')
        fig2.tight_layout()
        top_png = os.path.join(outdir, 'top_subtree_elapsed.png')
        fig2.savefig(top_png)
        plt.close(fig2)

        print('depth plot written to', depth_png)
        print('top-subtree plot written to', top_png)
    except Exception as e:
        print('matplotlib not available or plotting failed:', e)


if __name__ == '__main__':
    main()
