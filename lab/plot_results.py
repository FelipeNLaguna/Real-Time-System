"""Auxiliary program for lab 4: timing statistics and plots.

Reads the files written by ./main for each scenario prefix:
    <prefix>_u.dat         t_us  u1  u2
    <prefix>_sim.dat       t_us  y1  y2  y3     -> y(t)
    <prefix>_sampling.dat  t_us  y1  y2  y3     -> y(k)

Computes the real period T(k) = t(k) - t(k-1) and the jitter
J(k) = T(k) - T_nominal for each task, prints a statistics table, and saves
the table (CSV) and all figures to ./figures.

Usage: python3 plot_results.py [noload_prefix] [load_prefix]
       (defaults: noload load)
"""

import os
import sys

import matplotlib.pyplot as plt
import pandas as pd

OUTPUT_DIR = "figures"

# Nominal period of each task, in ms
TASKS = {
    "u": {"label": "gen_u", "period_ms": 50.0, "columns": ["t_us", "u1", "u2"]},
    "sim": {"label": "simulator", "period_ms": 30.0, "columns": ["t_us", "y1", "y2", "y3"]},
    "sampling": {"label": "sampling", "period_ms": 50.0, "columns": ["t_us", "y1", "y2", "y3"]},
}

# One color per scenario (validated categorical slots 1 and 2)
SCENARIO_STYLE = {
    "no load": {"color": "#2a78d6"},
    "with load": {"color": "#eb6834"},
}
REFERENCE_COLOR = "#52514e"

Y_LABELS = {"y1": "y1 [m]", "y2": "y2 [m]", "y3": "y3 [rad]"}
U_LABELS = {"u1": "u1 = v [m/s]", "u2": "u2 = ω [rad/s]"}

plt.rcParams.update({
    "figure.dpi": 150,
    "axes.grid": True,
    "grid.color": "#e4e3df",
    "grid.linewidth": 0.6,
    "axes.edgecolor": "#b5b4ae",
    "axes.labelcolor": "#0b0b0b",
    "xtick.color": "#52514e",
    "ytick.color": "#52514e",
    "axes.spines.top": False,
    "axes.spines.right": False,
    "lines.linewidth": 1.2,
    "legend.frameon": False,
})


def load_task(prefix, task):
    """Read one task file and add time, period and jitter columns."""
    path = f"{prefix}_{task}.dat"
    df = pd.read_csv(path, sep=r"\s+", header=None, names=TASKS[task]["columns"])
    df["t_s"] = df["t_us"] / 1e6
    df["T_ms"] = df["t_us"].diff() / 1000.0
    df["J_ms"] = df["T_ms"] - TASKS[task]["period_ms"]
    return df


def load_scenario(prefix):
    """Return {task: DataFrame}, or None if any file of the scenario is missing."""
    missing = [f"{prefix}_{t}.dat" for t in TASKS if not os.path.exists(f"{prefix}_{t}.dat")]
    if missing:
        print(f"warning: skipping prefix '{prefix}', missing: {', '.join(missing)}", file=sys.stderr)
        return None
    return {task: load_task(prefix, task) for task in TASKS}


def statistics_table(scenarios):
    """Mean, variance, std, min and max of T(k) and J(k) per scenario and task."""
    rows = []
    for scenario, data in scenarios.items():
        for task, df in data.items():
            row = {"scenario": scenario, "task": TASKS[task]["label"]}
            for metric in ("T_ms", "J_ms"):
                values = df[metric].dropna()
                name = metric.split("_")[0]
                row[f"{name} mean"] = values.mean()
                row[f"{name} var"] = values.var()
                row[f"{name} std"] = values.std()
                row[f"{name} min"] = values.min()
                row[f"{name} max"] = values.max()
            rows.append(row)
    return pd.DataFrame(rows).set_index(["scenario", "task"])


def save(fig, name):
    path = os.path.join(OUTPUT_DIR, name)
    fig.tight_layout()
    fig.savefig(path)
    plt.close(fig)
    print(f"saved {path}")


def plot_signals(scenarios, task, columns, labels, title, filename, step=False):
    fig, axes = plt.subplots(len(columns), 1, figsize=(8, 2.2 * len(columns)), sharex=True)
    for ax, column in zip(axes, columns):
        for scenario, data in scenarios.items():
            df = data[task]
            style = SCENARIO_STYLE[scenario]
            if step:
                ax.step(df["t_s"], df[column], where="post", color=style["color"], label=scenario)
            else:
                ax.plot(df["t_s"], df[column], color=style["color"], label=scenario)
        ax.set_ylabel(labels[column])
    axes[0].set_title(title, loc="left")
    axes[-1].set_xlabel("t [s]")
    if len(scenarios) > 1:
        axes[0].legend(loc="upper right")
    save(fig, filename)


def plot_timing(scenarios, metric, ylabel, title, filename):
    """One subplot per task; scenarios overlaid; dashed reference line."""
    fig, axes = plt.subplots(len(TASKS), 1, figsize=(8, 2.4 * len(TASKS)))
    # Draw "with load" first so the usually tighter "no load" trace stays visible
    order = sorted(scenarios, key=lambda s: s != "with load")
    for ax, task in zip(axes, TASKS):
        nominal = TASKS[task]["period_ms"]
        reference = nominal if metric == "T_ms" else 0.0
        ax.axhline(reference, color=REFERENCE_COLOR, linewidth=0.8, linestyle="--",
                   label=f"nominal ({nominal:g} ms)" if metric == "T_ms" else "zero jitter")
        for scenario in order:
            df = scenarios[scenario][task]
            ax.plot(df.index, df[metric], color=SCENARIO_STYLE[scenario]["color"],
                    linewidth=0.9, label=scenario)
        ax.set_ylabel(ylabel)
        ax.set_title(f"{TASKS[task]['label']} (T = {nominal:g} ms)", loc="left", fontsize=10)
        ax.legend(loc="upper right", fontsize=8)
    axes[0].figure.suptitle(title, x=0.01, ha="left")
    axes[-1].set_xlabel("k")
    save(fig, filename)


def main():
    prefixes = sys.argv[1:3] if len(sys.argv) > 1 else ["noload", "load"]
    names = ["no load", "with load"]

    scenarios = {}
    for name, prefix in zip(names, prefixes):
        data = load_scenario(prefix)
        if data is not None:
            scenarios[name] = data
    if not scenarios:
        print("error: no scenario could be loaded", file=sys.stderr)
        return 1

    os.makedirs(OUTPUT_DIR, exist_ok=True)

    table = statistics_table(scenarios)
    print("\nPeriod T(k) and jitter J(k) statistics [ms; var in ms²]\n")
    print(table.round(4).to_string())
    table.round(4).to_csv(os.path.join(OUTPUT_DIR, "timing_statistics.csv"))
    print()

    plot_signals(scenarios, "u", ["u1", "u2"], U_LABELS, "Input u(t)", "u_t.png", step=True)
    plot_signals(scenarios, "sim", ["y1", "y2", "y3"], Y_LABELS, "Output y(t) (simulator, 30 ms)", "y_t.png")
    plot_signals(scenarios, "sampling", ["y1", "y2", "y3"], Y_LABELS,
                 "Sampled output y(k) (sampling, 50 ms)", "y_k.png", step=True)
    plot_timing(scenarios, "T_ms", "T(k) [ms]", "Real period T(k)", "period.png")
    plot_timing(scenarios, "J_ms", "J(k) [ms]", "Jitter J(k) = T(k) − T", "jitter.png")
    return 0


if __name__ == "__main__":
    sys.exit(main())
