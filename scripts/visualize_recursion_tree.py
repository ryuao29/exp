#!/usr/bin/env python3
"""
Visualize BK recursion tree from CSV log.

Usage:
  python3 scripts/visualize_recursion_tree.py log.csv [--output-dir analyze]

Outputs:
  - recursion_tree_ascii.txt
  - recursion_tree_mermaid.mmd
  - recursion_tree_view.md
"""

import csv
import os
import sys
from collections import defaultdict


def normalize_parent(raw_parent: str) -> int:
    try:
        parent_val = int(raw_parent)
    except Exception:
        return -1
    if raw_parent.strip() == "-1" or parent_val >= (1 << 60):
        return -1
    return parent_val


def read_nodes(csv_path: str):
    nodes = {}
    children = defaultdict(list)

    with open(csv_path, newline="") as f:
        reader = csv.DictReader(f)
        for raw_row in reader:
            row = {k.strip(): (v.strip() if isinstance(v, str) else v) for k, v in raw_row.items()}
            nid = int(row["node_id"])
            parent = normalize_parent(row.get("parent_id", "-1"))
            depth = int(row.get("depth", "0") or 0)
            clique_vertices = row.get("clique_vertices", "")
            p_size = int(row.get("p_size", "0") or 0)
            x_size = int(row.get("x_size", "0") or 0)
            elapsed_us = int(row.get("elapsed_us", "0") or 0)
            is_leaf = int(row.get("is_leaf", "0") or 0)

            nodes[nid] = {
                "node_id": nid,
                "parent_id": parent,
                "depth": depth,
                "clique_vertices": clique_vertices,
                "p_size": p_size,
                "x_size": x_size,
                "elapsed_us": elapsed_us,
                "is_leaf": is_leaf,
            }
            if parent != -1:
                children[parent].append(nid)

    for parent, kid_list in children.items():
        kid_list.sort()

    roots = sorted(
        nid
        for nid, n in nodes.items()
        if n["parent_id"] == -1 or n["parent_id"] not in nodes
    )
    return nodes, children, roots


def format_path_from_clique(clique_vertices: str) -> str:
    if not clique_vertices:
        return "{}"
    return clique_vertices.replace(" ", "-")


def short_label(node, path_text=None):
    clique = node["clique_vertices"] if node["clique_vertices"] else "{}"
    leaf_mark = " leaf" if node["is_leaf"] == 1 else ""
    path_mark = f" path={path_text}" if path_text else ""
    return (
        f"id={node['node_id']} R={clique} d={node['depth']} "
        f"P={node['p_size']} X={node['x_size']} t={node['elapsed_us']}us{leaf_mark}{path_mark}"
    )


def render_ascii(nodes, children, roots):
    lines = []

    def visit(nid, prefix, is_last):
        conn = "`-- " if is_last else "|-- "
        lines.append(prefix + conn + short_label(nodes[nid], format_path_from_clique(nodes[nid]["clique_vertices"])))
        next_prefix = prefix + ("    " if is_last else "|   ")
        kids = children.get(nid, [])
        for i, kid in enumerate(kids):
            visit(kid, next_prefix, i == len(kids) - 1)

    if not roots:
        return "(empty tree)\n"

    for r_i, root in enumerate(roots):
        lines.append(short_label(nodes[root], format_path_from_clique(nodes[root]["clique_vertices"])))
        kids = children.get(root, [])
        for i, kid in enumerate(kids):
            visit(kid, "", i == len(kids) - 1)
        if r_i != len(roots) - 1:
            lines.append("")
    return "\n".join(lines) + "\n"


def escape_mermaid_label(s: str) -> str:
    return s.replace('"', "'")


def render_mermaid(nodes, children, roots):
    lines = ["flowchart TD"]

    for nid in sorted(nodes.keys()):
        node = nodes[nid]
        label = escape_mermaid_label(short_label(node, format_path_from_clique(node["clique_vertices"])))
        lines.append(f"  N{nid}[\"{label}\"]")

    for parent in sorted(children.keys()):
        for kid in children[parent]:
            lines.append(f"  N{parent} --> N{kid}")

    leaf_ids = [nid for nid, n in nodes.items() if n["is_leaf"] == 1]
    if leaf_ids:
        lines.append("  classDef leaf fill:#dff7df,stroke:#2e7d32,color:#1b5e20")
        lines.append("  class " + ",".join(f"N{nid}" for nid in sorted(leaf_ids)) + " leaf")

    if roots:
        lines.append("  classDef root fill:#e3f2fd,stroke:#1565c0,color:#0d47a1")
        lines.append("  class " + ",".join(f"N{nid}" for nid in roots) + " root")

    return "\n".join(lines) + "\n"


def render_markdown(ascii_tree: str, mermaid_tree: str, csv_path: str):
    return (
        "# Recursion Tree View\n\n"
        f"Source CSV: `{csv_path}`\n\n"
        "## Mermaid\n\n"
        "```mermaid\n"
        f"{mermaid_tree}"
        "```\n\n"
        "## ASCII Tree\n\n"
        "```text\n"
        f"{ascii_tree}"
        "```\n"
    )


def main():
    if len(sys.argv) < 2:
        print("usage: visualize_recursion_tree.py /path/to/log.csv [--output-dir DIR]")
        sys.exit(2)

    csv_path = sys.argv[1]
    outdir = "analyze"
    if "--output-dir" in sys.argv:
        i = sys.argv.index("--output-dir")
        if i + 1 < len(sys.argv):
            outdir = sys.argv[i + 1]

    if not os.path.exists(csv_path):
        print("error: log file not found:", csv_path)
        sys.exit(3)

    nodes, children, roots = read_nodes(csv_path)
    if not nodes:
        print("no data in log")
        sys.exit(0)

    os.makedirs(outdir, exist_ok=True)
    ascii_tree = render_ascii(nodes, children, roots)
    mermaid_tree = render_mermaid(nodes, children, roots)
    markdown_view = render_markdown(ascii_tree, mermaid_tree, csv_path)

    ascii_path = os.path.join(outdir, "recursion_tree_ascii.txt")
    mermaid_path = os.path.join(outdir, "recursion_tree_mermaid.mmd")
    markdown_path = os.path.join(outdir, "recursion_tree_view.md")

    with open(ascii_path, "w", encoding="utf-8") as f:
        f.write(ascii_tree)
    with open(mermaid_path, "w", encoding="utf-8") as f:
        f.write(mermaid_tree)
    with open(markdown_path, "w", encoding="utf-8") as f:
        f.write(markdown_view)

    print("nodes:", len(nodes))
    print("roots:", len(roots))
    print("ascii tree written to", ascii_path)
    print("mermaid tree written to", mermaid_path)
    print("markdown view written to", markdown_path)


if __name__ == "__main__":
    main()
