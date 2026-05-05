#!/usr/bin/env python3
"""
plot.py – Generate comparison plots for Jen-Schmidt vs Slota.

For each of the 7 dataset types a single image with 2×2 subplots is saved:
  ┌───────────────────────┬───────────────────────┐
  │ Memory  vs  Vertices  │ Memory  vs  Edges     │
  ├───────────────────────┼───────────────────────┤
  │ Time    vs  Vertices  │ Time    vs  Edges     │
  └───────────────────────┴───────────────────────┘

Output: code/plots/<dataset_type>.png   (7 images total)
"""

import os
import re
import glob
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
from matplotlib.lines import Line2D

# ─── Paths ────────────────────────────────────────────────────────────────────
SCRIPT_DIR  = os.path.dirname(os.path.abspath(__file__))
RESULTS_DIR = os.path.join(SCRIPT_DIR, "results")
PLOTS_DIR   = os.path.join(SCRIPT_DIR, "plots")

DATASET_TYPES = [
    "dense",
    "highly_connected",
    "large",
    "real_world",
    "small",
    "sparse",
    "tree_like",
]

# Map display name → file suffix used in results/
ALGORITHMS = {
    "Jen-Schmidt": "jen",
    "Slota":       "slota",
}

# ─── Colours / style ─────────────────────────────────────────────────────────
ALG_STYLE = {
    "Jen-Schmidt": dict(color="#6C63FF", marker="o", lw=2.2, ms=7),
    "Slota":       dict(color="#FF6584", marker="s", lw=2.2, ms=7),
}

# ─── Parser ───────────────────────────────────────────────────────────────────
# Matches lines like:
#   small_01.txt: 0.003423 seconds, Memory : 368 Bytes, Vertices: 3, Edges: 2
_LINE_RE = re.compile(
    r"^(?P<fname>\S+):\s+"
    r"(?P<time>[0-9.]+)\s+seconds,\s+"
    r"Memory\s*:\s*(?P<mem>\d+)\s+[Bb]ytes,\s+"
    r"Vertices:\s*(?P<v>\d+),\s+"
    r"Edges:\s*(?P<e>\d+)"
)


def parse_result_file(path: str) -> list[dict]:
    """Return a list of dicts with keys: fname, time, memory, vertices, edges."""
    records = []
    with open(path, "r") as f:
        for line in f:
            m = _LINE_RE.match(line.strip())
            if m:
                records.append({
                    "fname":    m.group("fname"),
                    "time":     float(m.group("time")),
                    "memory":   int(m.group("mem")),
                    "vertices": int(m.group("v")),
                    "edges":    int(m.group("e")),
                })
    return records


def load_dataset_type(dtype: str) -> dict[str, list[dict]]:
    """Load results for all algorithms for a single dataset type."""
    data = {}
    for alg_name, alg_suffix in ALGORITHMS.items():
        path = os.path.join(RESULTS_DIR, f"{dtype}_{alg_suffix}.txt")
        if not os.path.exists(path):
            print(f"  [WARN] missing {path}")
            data[alg_name] = []
        else:
            data[alg_name] = parse_result_file(path)
    return data


# ─── Plotting ─────────────────────────────────────────────────────────────────
def _sorted_xy(records: list[dict], x_key: str, y_key: str):
    """Return (x_vals, y_vals) sorted by x."""
    pairs = sorted([(r[x_key], r[y_key]) for r in records], key=lambda p: p[0])
    xs = [p[0] for p in pairs]
    ys = [p[1] for p in pairs]
    return xs, ys


def _human_bytes(x, _pos=None):
    """Formatter: bytes → KB / MB."""
    if x >= 1_000_000:
        return f"{x/1_000_000:.1f} MB"
    elif x >= 1_000:
        return f"{x/1_000:.0f} KB"
    return f"{int(x)} B"


def _human_ms(x, _pos=None):
    """Formatter: seconds → ms or s."""
    if x >= 1.0:
        return f"{x:.2f} s"
    return f"{x*1000:.1f} ms"


def plot_dataset(dtype: str, alg_data: dict[str, list[dict]], out_path: str):
    fig, axes = plt.subplots(
        2, 2,
        figsize=(14, 10),
        facecolor="#0F0F1A",
    )
    fig.patch.set_facecolor("#0F0F1A")

    # Title
    label = dtype.replace("_", " ").title()
    fig.suptitle(
        f"Algorithm Comparison — {label} Graphs",
        fontsize=18, fontweight="bold", color="white",
        y=0.97,
    )

    subplot_configs = [
        # (ax, x_key,    y_key,     x_label,    y_label,       y_formatter)
        (axes[0, 0], "vertices", "memory",   "Vertices",  "Memory",      _human_bytes),
        (axes[0, 1], "edges",    "memory",   "Edges",     "Memory",      _human_bytes),
        (axes[1, 0], "vertices", "time",     "Vertices",  "Time Taken",  _human_ms),
        (axes[1, 1], "edges",    "time",     "Edges",     "Time Taken",  _human_ms),
    ]

    for ax, x_key, y_key, x_label, y_label, yfmt in subplot_configs:
        ax.set_facecolor("#1A1A2E")
        ax.tick_params(colors="#CCCCCC", labelsize=9)
        for spine in ax.spines.values():
            spine.set_edgecolor("#333355")

        ax.set_xlabel(x_label, color="#AAAACC", fontsize=11, labelpad=6)
        ax.set_ylabel(y_label, color="#AAAACC", fontsize=11, labelpad=6)
        ax.set_title(
            f"{y_label} vs {x_label}",
            color="white", fontsize=12, pad=8,
        )
        ax.yaxis.set_major_formatter(ticker.FuncFormatter(yfmt))
        ax.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x, _: f"{int(x):,}"))
        ax.grid(True, linestyle="--", linewidth=0.5, alpha=0.35, color="#4444AA")

        for alg in ALGORITHMS.keys():
            records = alg_data.get(alg, [])
            if not records:
                continue
            xs, ys = _sorted_xy(records, x_key, y_key)
            style = ALG_STYLE[alg]
            ax.plot(
                xs, ys,
                label=alg,
                color=style["color"],
                marker=style["marker"],
                linewidth=style["lw"],
                markersize=style["ms"],
                markeredgecolor="white",
                markeredgewidth=0.6,
                zorder=3,
            )

    # Shared legend
    legend_elements = [
        Line2D([0], [0],
               color=ALG_STYLE[alg]["color"],
               marker=ALG_STYLE[alg]["marker"],
               linewidth=2, markersize=8,
               markeredgecolor="white", markeredgewidth=0.6,
               label=alg)
        for alg in ALGORITHMS.keys()
    ]
    fig.legend(
        handles=legend_elements,
        loc="lower center",
        ncol=2,
        fontsize=12,
        framealpha=0.2,
        labelcolor="white",
        facecolor="#1A1A2E",
        edgecolor="#333355",
        bbox_to_anchor=(0.5, 0.01),
    )

    plt.tight_layout(rect=[0, 0.06, 1, 0.96])
    fig.savefig(out_path, dpi=150, bbox_inches="tight", facecolor=fig.get_facecolor())
    plt.close(fig)
    print(f"  → Saved: {out_path}")


# ─── Main ─────────────────────────────────────────────────────────────────────
def main():
    os.makedirs(PLOTS_DIR, exist_ok=True)

    for dtype in DATASET_TYPES:
        print(f"\n[{dtype}]")
        alg_data = load_dataset_type(dtype)

        # Report how many valid data points each algorithm has
        for alg, recs in alg_data.items():
            print(f"  {alg}: {len(recs)} data points")

        out_path = os.path.join(PLOTS_DIR, f"{dtype}.png")
        plot_dataset(dtype, alg_data, out_path)

    print(f"\n✓ All plots saved to: {PLOTS_DIR}")


if __name__ == "__main__":
    main()
