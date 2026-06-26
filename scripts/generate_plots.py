#!/usr/bin/env python3
"""Generate SVG plots from benchmark CSV files.

The script intentionally uses only Python's standard library so the plots can be
recreated in a clean Windows/MSYS2 environment without installing plotting
packages.  Run the benchmarks first, or use `mingw32-make plots`.
"""

from __future__ import annotations

import csv
import html
import math
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "data"
PLOTS = ROOT / "plots"
WIDTH = 900
HEIGHT = 540
LEFT = 92
RIGHT = 36
TOP = 54
BOTTOM = 78
COLORS = ["#2563eb", "#dc2626", "#16a34a", "#9333ea"]


def read_csv(name: str) -> list[dict[str, str]]:
    path = DATA / name
    if not path.exists():
        raise FileNotFoundError(f"arquivo ausente: {path}")
    with path.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    if not rows:
        raise ValueError(f"arquivo sem dados: {path}")
    return rows


def as_float(row: dict[str, str], key: str) -> float:
    try:
        return float(row[key])
    except KeyError as error:
        raise ValueError(f"coluna ausente no CSV: {key}") from error


def mean(values: list[float]) -> float:
    return sum(values) / len(values)


def grouped_mean(rows: list[dict[str, str]], key: str,
                 value: str) -> list[tuple[float, float]]:
    groups: dict[float, list[float]] = defaultdict(list)
    for row in rows:
        groups[as_float(row, key)].append(as_float(row, value))
    return [(item, mean(groups[item])) for item in sorted(groups)]


def extent(series: list[tuple[str, list[tuple[float, float]]]],
           log_x: bool) -> tuple[float, float, float, float]:
    xs: list[float] = []
    ys: list[float] = []
    for _, points in series:
        for x_value, y_value in points:
            xs.append(math.log10(x_value) if log_x else x_value)
            ys.append(y_value)
    if not xs or not ys:
        raise ValueError("grafico sem pontos")
    x_min = min(xs)
    x_max = max(xs)
    y_min = 0.0 if min(ys) >= 0.0 else min(ys)
    y_max = max(ys)
    if x_min == x_max:
        x_min -= 1.0
        x_max += 1.0
    if y_min == y_max:
        y_max = y_min + 1.0
    return x_min, x_max, y_min, y_max


def fmt(value: float) -> str:
    if abs(value) >= 1_000_000_000:
        return f"{value / 1_000_000_000:.1f}G"
    if abs(value) >= 1_000_000:
        return f"{value / 1_000_000:.1f}M"
    if abs(value) >= 1_000:
        return f"{value / 1_000:.1f}k"
    if value == int(value):
        return str(int(value))
    return f"{value:.2f}"


def point_to_svg(x_value: float, y_value: float,
                 bounds: tuple[float, float, float, float],
                 log_x: bool) -> tuple[float, float]:
    x_min, x_max, y_min, y_max = bounds
    x_plot = math.log10(x_value) if log_x else x_value
    x_ratio = (x_plot - x_min) / (x_max - x_min)
    y_ratio = (y_value - y_min) / (y_max - y_min)
    x_svg = LEFT + x_ratio * (WIDTH - LEFT - RIGHT)
    y_svg = HEIGHT - BOTTOM - y_ratio * (HEIGHT - TOP - BOTTOM)
    return x_svg, y_svg


def svg_header(title: str) -> list[str]:
    safe_title = html.escape(title)
    return [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" '
        f'height="{HEIGHT}" viewBox="0 0 {WIDTH} {HEIGHT}">',
        "<style>",
        "text{font-family:Segoe UI,Arial,sans-serif;font-size:13px;"
        "fill:#172033}",
        ".title{font-size:22px;font-weight:700}",
        ".axis{stroke:#172033;stroke-width:1.5}",
        ".grid{stroke:#d8dee9;stroke-width:1}",
        ".legend{font-size:12px}",
        "</style>",
        '<rect width="100%" height="100%" fill="#ffffff"/>',
        f'<text x="{WIDTH / 2:.0f}" y="30" text-anchor="middle" '
        f'class="title">{safe_title}</text>',
    ]


def svg_axes(lines: list[str], bounds: tuple[float, float, float, float],
             xlabel: str, ylabel: str, log_x: bool) -> None:
    x_min, x_max, y_min, y_max = bounds
    plot_width = WIDTH - LEFT - RIGHT
    plot_height = HEIGHT - TOP - BOTTOM
    lines.append(f'<line x1="{LEFT}" y1="{HEIGHT - BOTTOM}" '
                 f'x2="{WIDTH - RIGHT}" y2="{HEIGHT - BOTTOM}" '
                 'class="axis"/>')
    lines.append(f'<line x1="{LEFT}" y1="{TOP}" x2="{LEFT}" '
                 f'y2="{HEIGHT - BOTTOM}" class="axis"/>')
    for tick in range(6):
        x = LEFT + plot_width * tick / 5
        y = TOP + plot_height * tick / 5
        y_value = y_max - (y_max - y_min) * tick / 5
        if log_x:
            x_value = 10 ** (x_min + (x_max - x_min) * tick / 5)
        else:
            x_value = x_min + (x_max - x_min) * tick / 5
        lines.append(f'<line x1="{x:.2f}" y1="{TOP}" x2="{x:.2f}" '
                     f'y2="{HEIGHT - BOTTOM}" class="grid"/>')
        lines.append(f'<line x1="{LEFT}" y1="{y:.2f}" '
                     f'x2="{WIDTH - RIGHT}" y2="{y:.2f}" class="grid"/>')
        lines.append(f'<text x="{x:.2f}" y="{HEIGHT - BOTTOM + 24}" '
                     f'text-anchor="middle">{fmt(x_value)}</text>')
        lines.append(f'<text x="{LEFT - 10}" y="{y + 4:.2f}" '
                     f'text-anchor="end">{fmt(y_value)}</text>')
    lines.append(f'<text x="{WIDTH / 2:.0f}" y="{HEIGHT - 20}" '
                 f'text-anchor="middle">{html.escape(xlabel)}</text>')
    lines.append(f'<text transform="translate(20 {HEIGHT / 2:.0f}) '
                 'rotate(-90)" text-anchor="middle">'
                 f'{html.escape(ylabel)}</text>')


def write_line_plot(path: Path, title: str, xlabel: str, ylabel: str,
                    series: list[tuple[str, list[tuple[float, float]]]],
                    log_x: bool = False) -> None:
    bounds = extent(series, log_x)
    lines = svg_header(title)
    svg_axes(lines, bounds, xlabel, ylabel, log_x)
    for index, (label, points) in enumerate(series):
        color = COLORS[index % len(COLORS)]
        coords = [point_to_svg(x, y, bounds, log_x) for x, y in points]
        polyline = " ".join(f"{x:.2f},{y:.2f}" for x, y in coords)
        lines.append(f'<polyline fill="none" stroke="{color}" '
                     'stroke-width="2.5" points="'
                     f'{polyline}"/>')
        for x, y in coords:
            lines.append(f'<circle cx="{x:.2f}" cy="{y:.2f}" r="4" '
                         f'fill="{color}"/>')
        legend_y = TOP + 18 + index * 20
        lines.append(f'<rect x="{WIDTH - RIGHT - 168}" y="{legend_y - 10}" '
                     f'width="12" height="12" fill="{color}"/>')
        lines.append(f'<text x="{WIDTH - RIGHT - 150}" y="{legend_y}" '
                     f'class="legend">{html.escape(label)}</text>')
    lines.append("</svg>")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_bar_plot(path: Path, title: str, ylabel: str,
                   bars: list[tuple[str, float]]) -> None:
    if not bars:
        raise ValueError("grafico de barras sem dados")
    y_max = max(value for _, value in bars)
    if y_max <= 0:
        y_max = 1.0
    lines = svg_header(title)
    plot_width = WIDTH - LEFT - RIGHT
    plot_height = HEIGHT - TOP - BOTTOM
    lines.append(f'<line x1="{LEFT}" y1="{HEIGHT - BOTTOM}" '
                 f'x2="{WIDTH - RIGHT}" y2="{HEIGHT - BOTTOM}" '
                 'class="axis"/>')
    lines.append(f'<line x1="{LEFT}" y1="{TOP}" x2="{LEFT}" '
                 f'y2="{HEIGHT - BOTTOM}" class="axis"/>')
    for tick in range(6):
        y = TOP + plot_height * tick / 5
        value = y_max - y_max * tick / 5
        lines.append(f'<line x1="{LEFT}" y1="{y:.2f}" '
                     f'x2="{WIDTH - RIGHT}" y2="{y:.2f}" class="grid"/>')
        lines.append(f'<text x="{LEFT - 10}" y="{y + 4:.2f}" '
                     f'text-anchor="end">{fmt(value)}</text>')
    lines.append(f'<text x="{WIDTH / 2:.0f}" y="{HEIGHT - 20}" '
                 'text-anchor="middle">algoritmo</text>')
    lines.append(f'<text transform="translate(20 {HEIGHT / 2:.0f}) '
                 'rotate(-90)" text-anchor="middle">'
                 f'{html.escape(ylabel)}</text>')
    slot = (WIDTH - LEFT - RIGHT) / len(bars)
    for index, (label, value) in enumerate(bars):
        color = COLORS[index % len(COLORS)]
        height = value / y_max * plot_height
        x = LEFT + slot * index + slot * 0.25
        y = HEIGHT - BOTTOM - height
        width = slot * 0.5
        lines.append(f'<rect x="{x:.2f}" y="{y:.2f}" width="{width:.2f}" '
                     f'height="{height:.2f}" fill="{color}"/>')
        lines.append(f'<text x="{x + width / 2:.2f}" '
                     f'y="{HEIGHT - BOTTOM + 46}" text-anchor="middle">'
                     f'{html.escape(label)}</text>')
        lines.append(f'<text x="{x + width / 2:.2f}" y="{y - 8:.2f}" '
                     f'text-anchor="middle">{fmt(value)}</text>')
    lines.append("</svg>")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    PLOTS.mkdir(parents=True, exist_ok=True)
    scale = read_csv("scale.csv")
    fire = read_csv("fire.csv")
    tree = read_csv("tree.csv")
    collectors = read_csv("collectors.csv")

    write_line_plot(
        PLOTS / "pause_vs_heap.svg",
        "Pausa do coletor por tamanho do heap",
        "heap_bytes (escala log10)",
        "pause_ticks",
        [("pausa", grouped_mean(scale, "heap_bytes", "pause_ticks"))],
        log_x=True,
    )
    write_line_plot(
        PLOTS / "memory_vs_progress.svg",
        "Memoria utilizada ao longo da carga",
        "ciclo do teste de fogo",
        "bytes",
        [
            ("heap reservado", grouped_mean(fire, "seed", "heap_bytes")),
            ("RSS maximo", grouped_mean(fire, "seed", "max_rss_bytes")),
        ],
    )
    write_line_plot(
        PLOTS / "tree_cost_vs_objects.svg",
        "Custo da arvore por quantidade de objetos",
        "objetos (escala log10)",
        "ticks",
        [
            ("insercao", grouped_mean(tree, "nodes", "insert_ticks")),
            ("busca", grouped_mean(tree, "nodes", "find_ticks")),
            ("remocao", grouped_mean(tree, "nodes", "remove_ticks")),
        ],
        log_x=True,
    )
    write_bar_plot(
        PLOTS / "collector_pause_comparison.svg",
        "Pausa media por algoritmo",
        "mean_pause_ticks",
        [(row["algorithm"], as_float(row, "mean_pause_ticks"))
         for row in collectors],
    )
    for path in sorted(PLOTS.glob("*.svg")):
        print(path.relative_to(ROOT))


if __name__ == "__main__":
    main()
