#!/usr/bin/env python3
"""Generate README benchmark charts from Google Benchmark JSON output."""

from __future__ import annotations

import argparse
import html
import json
import math
import re
from collections import defaultdict
from pathlib import Path
from statistics import median

DATASET_NAMES = {
    0: "canada",
    1: "citm_catalog",
    2: "fgo",
    3: "github_events",
    4: "gsoc-2018",
    5: "lottie",
    6: "otfcc",
    7: "poet",
    8: "twitter",
    9: "twitterescaped",
}

READ_COLORS = {
    "cpp_yyjson": "#2563eb",
    "c_yyjson": "#16a34a",
    "rapidjson": "#dc2626",
    "simdjson": "#9333ea",
    "glaze": "#ca8a04",
}

CONTRACTS = {
    "fixed": ("fixed_input", "Read-only fixed-length input"),
    "prepared": ("prepared_input", "Caller-prepared writable/padded buffer"),
}

POLICIES = {
    "fresh": "New parser / allocator per parse",
    "reused": "Reused parser / allocator",
}

# benchmark name -> (input contract, allocation policy, color key, label)
READ_VARIANTS = {
    "read_cpp_yyjson": ("fixed", "fresh", "cpp_yyjson", "cpp-yyjson"),
    "read_cpp_yyjson_insitu_copy": (
        "fixed",
        "fresh",
        "cpp_yyjson",
        "cpp-yyjson (copy + in-situ)",
    ),
    "read_cpp_yyjson_single": ("fixed", "reused", "cpp_yyjson", "cpp-yyjson"),
    "read_cpp_yyjson_insitu_single_copy": (
        "fixed",
        "reused",
        "cpp_yyjson",
        "cpp-yyjson (copy + in-situ)",
    ),
    "read_cpp_yyjson_insitu": (
        "prepared",
        "fresh",
        "cpp_yyjson",
        "cpp-yyjson (in-situ)",
    ),
    "read_cpp_yyjson_insitu_single": (
        "prepared",
        "reused",
        "cpp_yyjson",
        "cpp-yyjson (in-situ)",
    ),
    "read_c_yyjson": ("fixed", "fresh", "c_yyjson", "yyjson"),
    "read_c_yyjson_insitu_copy": (
        "fixed",
        "fresh",
        "c_yyjson",
        "yyjson (copy + in-situ)",
    ),
    "read_c_yyjson_single": ("fixed", "reused", "c_yyjson", "yyjson"),
    "read_c_yyjson_insitu_single_copy": (
        "fixed",
        "reused",
        "c_yyjson",
        "yyjson (copy + in-situ)",
    ),
    "read_c_yyjson_insitu": ("prepared", "fresh", "c_yyjson", "yyjson (in-situ)"),
    "read_c_yyjson_insitu_single": (
        "prepared",
        "reused",
        "c_yyjson",
        "yyjson (in-situ)",
    ),
    "read_rapidjson": ("fixed", "fresh", "rapidjson", "rapidjson"),
    "read_rapidjson_insitu_copy": (
        "fixed",
        "fresh",
        "rapidjson",
        "rapidjson (copy + in-situ)",
    ),
    "read_rapidjson_single": ("fixed", "reused", "rapidjson", "rapidjson"),
    "read_rapidjson_insitu_single_copy": (
        "fixed",
        "reused",
        "rapidjson",
        "rapidjson (copy + in-situ)",
    ),
    "read_rapidjson_insitu": (
        "prepared",
        "fresh",
        "rapidjson",
        "rapidjson (in-situ)",
    ),
    "read_rapidjson_insitu_single": (
        "prepared",
        "reused",
        "rapidjson",
        "rapidjson (in-situ)",
    ),
    "read_simdjson_dom": ("fixed", "fresh", "simdjson", "simdjson DOM"),
    "read_simdjson_dom_pad_copy": (
        "fixed",
        "fresh",
        "simdjson",
        "simdjson DOM (copy + pad)",
    ),
    "read_simdjson_ond_pad_copy": (
        "fixed",
        "fresh",
        "simdjson",
        "simdjson On Demand (copy + pad)",
    ),
    "read_simdjson_dom_single": ("fixed", "reused", "simdjson", "simdjson DOM"),
    "read_simdjson_dom_pad_single_copy": (
        "fixed",
        "reused",
        "simdjson",
        "simdjson DOM (copy + pad)",
    ),
    "read_simdjson_ond_pad_single_copy": (
        "fixed",
        "reused",
        "simdjson",
        "simdjson On Demand (copy + pad)",
    ),
    "read_simdjson_dom_pad": ("prepared", "fresh", "simdjson", "simdjson DOM (pad)"),
    "read_simdjson_ond_pad": (
        "prepared",
        "fresh",
        "simdjson",
        "simdjson On Demand (pad)",
    ),
    "read_simdjson_dom_pad_single": (
        "prepared",
        "reused",
        "simdjson",
        "simdjson DOM (pad)",
    ),
    "read_simdjson_ond_pad_single": (
        "prepared",
        "reused",
        "simdjson",
        "simdjson On Demand (pad)",
    ),
    "read_glaze": ("fixed", "fresh", "glaze", "Glaze"),
    "read_glaze_single": ("fixed", "reused", "glaze", "Glaze"),
}

WRITE_COLORS = {
    "cpp-yyjson": "#2563eb",
    "cpp-yyjson range": "#2563eb",
    "cpp-yyjson single": "#2563eb",
    "cpp-yyjson reflection": "#2563eb",
    "cpp-yyjson macro": "#2563eb",
    "yyjson": "#16a34a",
    "rapidjson": "#dc2626",
    "nlohmann-json": "#f97316",
    "Glaze": "#ca8a04",
}

WRITE_POLICIES = {
    "fresh": "New output buffer per write",
    "reused": "Reused output buffer",
}

# benchmark name -> (output buffer policy, color key, label)
WRITE_DATASET_VARIANTS = {
    "write_cpp_yyjson_dataset": ("fresh", "cpp-yyjson", "cpp-yyjson"),
    "write_cpp_yyjson_single_dataset": ("reused", "cpp-yyjson", "cpp-yyjson"),
    "write_c_yyjson_dataset": ("fresh", "yyjson", "yyjson"),
    "write_c_yyjson_single_dataset": ("reused", "yyjson", "yyjson"),
    "write_rapidjson_dataset": ("fresh", "rapidjson", "rapidjson"),
    "write_rapidjson_single_dataset": ("reused", "rapidjson", "rapidjson"),
    "write_nlohmann_dataset": ("fresh", "nlohmann-json", "nlohmann-json"),
    "write_glaze_dataset": ("fresh", "Glaze", "Glaze"),
    "write_glaze_single_dataset": ("reused", "Glaze", "Glaze"),
}

# The typed benchmarks number their own datasets, in the order of `typed_datasets` in
# test/bench_typed.cpp.
TYPED_DATASET_NAMES = {
    0: "canada",
    1: "citm_catalog",
    2: "twitter",
    3: "twitterescaped",
    4: "poet",
    5: "github_events",
    6: "gsoc-2018",
}

TYPED_COLORS = {
    "cpp_yyjson": "#2563eb",
    "cpp_yyjson_dom": "#93c5fd",
    "glaze": "#ca8a04",
}

TYPED_POLICIES = {
    "read": {
        "fresh": "New allocator / object per read",
        "object": "Reused result object",
    },
    "write": WRITE_POLICIES,
}

# benchmark name -> (direction, policy, color key, label), or None for a row that is
# measured but not drawn.  The charts compare the data binding API of cpp-yyjson and its
# DOM API, which reads a document and casts it or builds one and writes it, with Glaze;
# the DOM read alone and the DOM read with a reused allocator have no Glaze counterpart.
# The "object" rows read into the object of the previous iteration and reuse the
# capacity of its strings and vectors.
TYPED_VARIANTS = {
    "typed_read_cpp_yyjson": ("read", "fresh", "cpp_yyjson_dom", "cpp-yyjson DOM: read + cast<T>"),
    "typed_read_cpp_yyjson_dom": None,
    "typed_read_cpp_yyjson_single": None,
    "typed_read_cpp_yyjson_direct": ("read", "fresh", "cpp_yyjson", "cpp-yyjson read<T>"),
    "typed_read_cpp_yyjson_direct_into": ("read", "object", "cpp_yyjson", "cpp-yyjson read_into"),
    "typed_read_cpp_yyjson_direct_merge": ("read", "object", "cpp_yyjson", "cpp-yyjson merge_into"),
    "typed_read_glaze": ("read", "fresh", "glaze", "Glaze glz::read"),
    "typed_read_glaze_single": ("read", "object", "glaze", "Glaze glz::read"),
    "typed_write_cpp_yyjson": ("write", "fresh", "cpp_yyjson_dom", "cpp-yyjson DOM: value(t).write()"),
    "typed_write_cpp_yyjson_single": (
        "write",
        "reused",
        "cpp_yyjson_dom",
        "cpp-yyjson DOM: value(t).write(alc)",
    ),
    "typed_write_cpp_yyjson_direct": ("write", "fresh", "cpp_yyjson", "cpp-yyjson write(t)"),
    "typed_write_cpp_yyjson_direct_single": ("write", "reused", "cpp_yyjson", "cpp-yyjson write(t, out)"),
    "typed_write_glaze": ("write", "fresh", "glaze", "Glaze glz::write"),
    "typed_write_glaze_single": ("write", "reused", "glaze", "Glaze glz::write"),
}

# The comparisons of the README tables: output name -> (title, subtitle,
# [(benchmark, color key, legend label)], colors, datasets).  The first benchmark is
# cpp-yyjson.
THROUGHPUT_CHARTS = {
    "read_throughput": (
        "Read throughput per document",
        "Read-only input, new parser / allocator per parse; higher is better",
        [
            ("read_cpp_yyjson", "cpp_yyjson", "cpp-yyjson"),
            ("read_c_yyjson", "c_yyjson", "yyjson"),
            (
                "read_simdjson_ond_pad_copy",
                "simdjson",
                "simdjson On Demand (copy + pad)",
            ),
            ("read_rapidjson", "rapidjson", "rapidjson"),
        ],
        READ_COLORS,
        DATASET_NAMES,
    ),
    "write_throughput": (
        "Write throughput per document",
        "Document serialization, new output buffer per write; higher is better",
        [
            ("write_cpp_yyjson_dataset", "cpp-yyjson", "cpp-yyjson"),
            ("write_c_yyjson_dataset", "yyjson", "yyjson"),
            ("write_rapidjson_dataset", "rapidjson", "rapidjson"),
            ("write_nlohmann_dataset", "nlohmann-json", "nlohmann-json"),
        ],
        WRITE_COLORS,
        DATASET_NAMES,
    ),
    "typed_read_throughput": (
        "Read throughput into C++ types",
        "New result object per read; higher is better",
        [
            ("typed_read_cpp_yyjson_direct", "cpp_yyjson", "cpp-yyjson read<T>"),
            ("typed_read_cpp_yyjson", "cpp_yyjson_dom", "cpp-yyjson DOM: read + cast<T>"),
            ("typed_read_glaze", "glaze", "Glaze glz::read"),
        ],
        TYPED_COLORS,
        TYPED_DATASET_NAMES,
    ),
    "typed_write_throughput": (
        "Write throughput from C++ types",
        "New output buffer per write; higher is better",
        [
            ("typed_write_cpp_yyjson_direct", "cpp_yyjson", "cpp-yyjson write(t)"),
            ("typed_write_cpp_yyjson", "cpp_yyjson_dom", "cpp-yyjson DOM: value(t).write()"),
            ("typed_write_glaze", "glaze", "Glaze glz::write"),
        ],
        TYPED_COLORS,
        TYPED_DATASET_NAMES,
    ),
}

TIME_UNITS = {"ns": 1e-9, "us": 1e-6, "ms": 1e-3, "s": 1.0}


def clean_name(name: str) -> str:
    cleaned = name.removesuffix("_median")
    cleaned = cleaned.removesuffix("_mean")
    cleaned = cleaned.removesuffix("_stddev")
    cleaned = cleaned.removesuffix("_cv")
    cleaned = cleaned.removesuffix("/manual_time")
    return cleaned


def benchmark_time(row: dict) -> float | None:
    value = row.get("real_time")
    if isinstance(value, int | float):
        return float(value)
    return None


def load_median_times(paths: list[Path]) -> dict[str, float]:
    aggregate_times: dict[str, float] = {}
    iteration_times: dict[str, list[float]] = defaultdict(list)

    for path in paths:
        with path.open(encoding="utf-8") as file:
            data = json.load(file)

        for row in data.get("benchmarks", []):
            name = row.get("name")
            if not isinstance(name, str):
                continue
            value = benchmark_time(row)
            if value is None:
                continue

            aggregate_name = row.get("aggregate_name")
            if aggregate_name == "median" or name.endswith("_median"):
                aggregate_times[clean_name(name)] = value
                continue

            if row.get("run_type") == "iteration":
                iteration_times[clean_name(name)].append(value)

    result = dict(aggregate_times)
    for name, values in iteration_times.items():
        result.setdefault(name, median(values))
    return result


def load_processed_bytes(paths: list[Path]) -> dict[str, float]:
    """Bytes processed per iteration, recovered from the `bytes_per_second` counter."""
    aggregate_bytes: dict[str, float] = {}
    iteration_bytes: dict[str, list[float]] = defaultdict(list)

    for path in paths:
        with path.open(encoding="utf-8") as file:
            data = json.load(file)

        for row in data.get("benchmarks", []):
            name = row.get("name")
            rate = row.get("bytes_per_second")
            time = benchmark_time(row)
            unit = TIME_UNITS.get(row.get("time_unit", "ms"))
            if not isinstance(name, str) or not isinstance(rate, int | float):
                continue
            if time is None or unit is None:
                continue
            value = rate * time * unit

            aggregate_name = row.get("aggregate_name")
            if aggregate_name == "median" or name.endswith("_median"):
                aggregate_bytes[clean_name(name)] = value
            elif row.get("run_type") == "iteration":
                iteration_bytes[clean_name(name)].append(value)

    result = dict(aggregate_bytes)
    for name, values in iteration_bytes.items():
        result.setdefault(name, median(values))
    return result


def collect_read_charts(
    times: dict[str, float],
) -> dict[tuple[str, str], list[tuple[str, list[tuple[str, float, str]]]]]:
    sections: dict[tuple[str, str, str], list[tuple[str, float, str]]] = defaultdict(
        list
    )
    pattern = re.compile(r"^(read_[^/]+)/(\d+)$")
    unknown_names: set[str] = set()
    unknown_datasets: set[int] = set()

    for name, value in times.items():
        match = pattern.match(name)
        if not match:
            unknown_names.add(name)
            continue
        benchmark_name = match.group(1)
        dataset_index = int(match.group(2))
        if benchmark_name not in READ_VARIANTS:
            unknown_names.add(benchmark_name)
            continue
        if dataset_index not in DATASET_NAMES:
            unknown_datasets.add(dataset_index)
            continue
        contract, policy, color_key, label = READ_VARIANTS[benchmark_name]
        key = (DATASET_NAMES[dataset_index], contract, policy)
        sections[key].append((label, value, color_key))

    if unknown_names:
        raise ValueError(
            "unmapped read benchmarks: " + ", ".join(sorted(unknown_names))
        )
    if unknown_datasets:
        raise ValueError(
            "unknown read dataset indexes: "
            + ", ".join(str(index) for index in sorted(unknown_datasets))
        )

    charts: dict[tuple[str, str], list[tuple[str, list[tuple[str, float, str]]]]] = {}
    for dataset_name in DATASET_NAMES.values():
        for contract in CONTRACTS:
            groups = []
            for policy, heading in POLICIES.items():
                rows = sections.get((dataset_name, contract, policy), [])
                if rows:
                    groups.append((heading, sorted(rows, key=lambda item: item[1])))
            if groups:
                charts[(dataset_name, contract)] = groups
    return charts


def write_case(name: str) -> tuple[str, str] | None:
    if name.startswith("write_cpp_yyjson_single_"):
        return name.removeprefix("write_cpp_yyjson_single_"), "cpp-yyjson single"
    if name == "write_cpp_yyjson_array_double_append_range":
        return "array_double_append", "cpp-yyjson range"
    if name.startswith("write_cpp_yyjson_"):
        rest = name.removeprefix("write_cpp_yyjson_")
        if rest.endswith("_reflection"):
            return rest.removesuffix("_reflection"), "cpp-yyjson reflection"
        if rest.endswith("_macro"):
            return rest.removesuffix("_macro"), "cpp-yyjson macro"
        return rest, "cpp-yyjson"
    if name.startswith("write_c_yyjson_"):
        return name.removeprefix("write_c_yyjson_"), "yyjson"
    return None


WRITE_DATASET_PATTERN = re.compile(r"^(write_[A-Za-z0-9_]*dataset)/(\d+)$")


def collect_write_dataset_charts(
    times: dict[str, float],
) -> dict[str, list[tuple[str, list[tuple[str, float, str]]]]]:
    sections: dict[tuple[str, str], list[tuple[str, float, str]]] = defaultdict(list)
    unknown_names: set[str] = set()
    unknown_datasets: set[int] = set()

    for name, value in times.items():
        match = WRITE_DATASET_PATTERN.match(name)
        if not match:
            continue
        benchmark_name = match.group(1)
        dataset_index = int(match.group(2))
        if benchmark_name not in WRITE_DATASET_VARIANTS:
            unknown_names.add(benchmark_name)
            continue
        if dataset_index not in DATASET_NAMES:
            unknown_datasets.add(dataset_index)
            continue
        policy, color_key, label = WRITE_DATASET_VARIANTS[benchmark_name]
        sections[(DATASET_NAMES[dataset_index], policy)].append((label, value, color_key))

    if unknown_names:
        raise ValueError(
            "unmapped write dataset benchmarks: " + ", ".join(sorted(unknown_names))
        )
    if unknown_datasets:
        raise ValueError(
            "unknown write dataset indexes: "
            + ", ".join(str(index) for index in sorted(unknown_datasets))
        )

    charts: dict[str, list[tuple[str, list[tuple[str, float, str]]]]] = {}
    for dataset_name in DATASET_NAMES.values():
        groups = []
        for policy, heading in WRITE_POLICIES.items():
            rows = sections.get((dataset_name, policy), [])
            if rows:
                groups.append((heading, sorted(rows, key=lambda item: item[1])))
        if groups:
            charts[dataset_name] = groups
    return charts


def collect_write_charts(
    times: dict[str, float],
) -> dict[str, list[tuple[str, list[tuple[str, float, str]]]]]:
    scenarios: dict[str, list[tuple[str, float, str]]] = defaultdict(list)
    unknown_names: set[str] = set()

    for name, value in times.items():
        if WRITE_DATASET_PATTERN.match(name):
            continue
        parsed = write_case(name)
        if parsed is None:
            unknown_names.add(name)
            continue
        scenario, implementation = parsed
        scenarios[scenario].append((implementation, value, implementation))

    if unknown_names:
        raise ValueError(
            "unmapped write benchmarks: " + ", ".join(sorted(unknown_names))
        )

    return {
        scenario: [("", sorted(rows, key=lambda item: item[1]))]
        for scenario, rows in sorted(scenarios.items())
    }


def collect_throughput(
    times: dict[str, float],
    processed_bytes: dict[str, float],
    benchmarks: list[str],
    datasets: dict[int, str],
) -> list[tuple[str, list[float]]]:
    # Every library is divided into the size cpp-yyjson reports, so the bars of a
    # document keep the time ratios of the README table even where the libraries
    # write texts of slightly different lengths.
    documents = []
    missing = []
    baseline = benchmarks[0]
    for index, dataset_name in datasets.items():
        size = processed_bytes.get(f"{baseline}/{index}")
        if size is None:
            continue
        values = []
        for benchmark in benchmarks:
            time = times.get(f"{benchmark}/{index}")
            if time is None:
                missing.append(f"{benchmark}/{index}")
            elif time > 0:
                values.append(size / (time * TIME_UNITS["ms"]) / 1e9)
        documents.append((dataset_name, values))
    if missing:
        raise ValueError("missing throughput benchmarks: " + ", ".join(missing))
    return documents


def nice_step(max_value: float, target_ticks: int = 5) -> float:
    raw = max_value / target_ticks
    magnitude = 10 ** math.floor(math.log10(raw))
    for factor in (1, 2, 2.5, 5, 10):
        if raw <= factor * magnitude:
            return factor * magnitude
    return 10 * magnitude


def write_throughput_svg(
    path: Path,
    title: str,
    subtitle: str,
    legend: list[tuple[str, str]],
    documents: list[tuple[str, list[float]]],
) -> None:
    values = [value for _, row in documents for value in row]
    if not values:
        return

    width = 980
    left = 64
    right = 24
    plot_top = 112
    plot_height = 280
    plot_bottom = plot_top + plot_height
    height = plot_bottom + 40
    group_width = (width - left - right) / len(documents)
    bar_gap = 0
    bar_width = group_width * 0.62 / len(legend)

    step = nice_step(max(values))
    scale_max = math.ceil(max(values) * 1.08 / step) * step

    def y_of(value: float) -> float:
        return plot_bottom - value / scale_max * plot_height

    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#ffffff"/>',
        "<style>text{font-family:ui-sans-serif,system-ui,-apple-system,BlinkMacSystemFont,&quot;Segoe UI&quot;,sans-serif;fill:#172033}.title{font-size:20px;font-weight:700}.subtitle{font-size:13px;fill:#475569}.label{font-size:13px}.value{font-size:12px;fill:#475569}.axis{stroke:#cbd5e1;stroke-width:1}.grid{stroke:#e2e8f0;stroke-width:1}</style>",
        f'<text x="24" y="34" class="title">{html.escape(title)}</text>',
        f'<text x="24" y="56" class="subtitle">{html.escape(subtitle)}</text>',
    ]

    legend_x = 24.0
    for label, color in legend:
        parts.append(
            f'<rect x="{legend_x:.1f}" y="72" width="12" height="12" rx="2" fill="{color}"/>'
        )
        parts.append(
            f'<text x="{legend_x + 18:.1f}" y="83" class="label">{html.escape(label)}</text>'
        )
        legend_x += 18 + 7.2 * len(label) + 24

    tick_count = round(scale_max / step)
    for tick_index in range(tick_count + 1):
        tick = step * tick_index
        tick_y = y_of(tick)
        parts.append(
            f'<line x1="{left}" y1="{tick_y:.1f}" x2="{width - right}" y2="{tick_y:.1f}" class="grid"/>'
        )
        parts.append(
            f'<text x="{left - 8}" y="{tick_y + 4:.1f}" class="value" text-anchor="end">{tick:g}</text>'
        )
    parts.append(f'<text x="24" y="{plot_top - 10}" class="value">GB/s</text>')
    parts.append(
        f'<line x1="{left}" y1="{plot_bottom}" x2="{width - right}" y2="{plot_bottom}" class="axis"/>'
    )

    for document_index, (dataset_name, row) in enumerate(documents):
        center = left + group_width * (document_index + 0.5)
        bars_width = bar_width * len(row) + bar_gap * (len(row) - 1)
        for value_index, value in enumerate(row):
            bar_x = center - bars_width / 2 + (bar_width + bar_gap) * value_index
            top = y_of(value)
            parts.append(
                f'<rect x="{bar_x:.1f}" y="{top:.1f}" width="{bar_width:.1f}" height="{max(plot_bottom - top, 1.0):.1f}" fill="{legend[value_index][1]}"/>'
            )
        parts.append(
            f'<text x="{center:.1f}" y="{plot_bottom + 20}" class="value" text-anchor="middle">{html.escape(dataset_name)}</text>'
        )

    parts.append("</svg>")
    path.write_text("\n".join(parts) + "\n", encoding="utf-8")


def generate_throughput_charts(
    times: dict[str, float],
    processed_bytes: dict[str, float],
    prefix: str,
    output_dir: Path,
) -> None:
    for name, chart in THROUGHPUT_CHARTS.items():
        title, subtitle, benchmarks, colors, datasets = chart
        if not name.startswith(prefix):
            continue
        documents = collect_throughput(
            times,
            processed_bytes,
            [benchmark for benchmark, _, _ in benchmarks],
            datasets,
        )
        if not documents:
            continue
        legend = [(label, colors[color_key]) for _, color_key, label in benchmarks]
        write_throughput_svg(
            output_dir / f"{name}.svg", title, subtitle, legend, documents
        )


def collect_typed_charts(
    times: dict[str, float],
) -> dict[tuple[str, str], list[tuple[str, list[tuple[str, float, str]]]]]:
    sections: dict[tuple[str, str, str], list[tuple[str, float, str]]] = defaultdict(
        list
    )
    pattern = re.compile(r"^(typed_[^/]+)/(\d+)$")
    unknown_names: set[str] = set()
    unknown_datasets: set[int] = set()

    for name, value in times.items():
        match = pattern.match(name)
        if not match or match.group(1) not in TYPED_VARIANTS:
            unknown_names.add(match.group(1) if match else name)
            continue
        dataset_index = int(match.group(2))
        if dataset_index not in TYPED_DATASET_NAMES:
            unknown_datasets.add(dataset_index)
            continue
        if TYPED_VARIANTS[match.group(1)] is None:
            continue
        direction, policy, color_key, label = TYPED_VARIANTS[match.group(1)]
        key = (TYPED_DATASET_NAMES[dataset_index], direction, policy)
        sections[key].append((label, value, color_key))

    if unknown_names:
        raise ValueError(
            "unmapped typed benchmarks: " + ", ".join(sorted(unknown_names))
        )
    if unknown_datasets:
        raise ValueError(
            "unknown typed dataset indexes: "
            + ", ".join(str(index) for index in sorted(unknown_datasets))
        )

    charts: dict[tuple[str, str], list[tuple[str, list[tuple[str, float, str]]]]] = {}
    for dataset_name in TYPED_DATASET_NAMES.values():
        for direction, policies in TYPED_POLICIES.items():
            groups = []
            for policy, heading in policies.items():
                rows = sections.get((dataset_name, direction, policy), [])
                if rows:
                    groups.append((heading, sorted(rows, key=lambda item: item[1])))
            if groups:
                charts[(dataset_name, direction)] = groups
    return charts


def safe_file_name(name: str) -> str:
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", name).strip("_")


def nice_title(name: str) -> str:
    return name.replace("_", " ")


def time_text(value: float) -> str:
    if value >= 100:
        return f"{value:.0f} ms"
    if value >= 10:
        return f"{value:.1f} ms"
    if value >= 1:
        return f"{value:.2f} ms"
    return f"{value:.3f} ms"


def write_svg(
    path: Path,
    title: str,
    groups: list[tuple[str, list[tuple[str, float, str]]]],
    colors: dict[str, str],
    min_rows: int = 0,
) -> None:
    rows_total = sum(len(rows) for _, rows in groups)
    if not rows_total:
        return

    width = 980
    row_height = 24
    heading_height = 26
    top = 58
    bottom = 28
    label_width = 300
    value_width = 86
    chart_width = width - label_width - value_width - 42
    heading_count = sum(1 for heading, _ in groups if heading)
    display_rows = max(rows_total, min_rows)
    height = top + bottom + row_height * display_rows + heading_height * heading_count
    max_value = max(value for _, rows in groups for _, value, _ in rows)
    scale_max = max_value * 1.08 if max_value > 0 else 1.0

    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#ffffff"/>',
        "<style>text{font-family:ui-sans-serif,system-ui,-apple-system,BlinkMacSystemFont,&quot;Segoe UI&quot;,sans-serif;fill:#172033}.title{font-size:20px;font-weight:700}.heading{font-size:13px;font-weight:600;fill:#334155}.label{font-size:13px}.value{font-size:12px;fill:#475569}.axis{stroke:#cbd5e1;stroke-width:1}.grid{stroke:#e2e8f0;stroke-width:1}</style>",
        f'<text x="24" y="34" class="title">{html.escape(title)}</text>',
    ]

    axis_x = label_width
    axis_y = top + row_height * display_rows + heading_height * heading_count + 2
    parts.append(
        f'<line x1="{axis_x}" y1="{axis_y}" x2="{axis_x + chart_width}" y2="{axis_y}" class="axis"/>'
    )

    for tick_index in range(5):
        tick_value = scale_max * tick_index / 4
        tick_x = axis_x + chart_width * tick_index / 4
        parts.append(
            f'<line x1="{tick_x:.1f}" y1="{top - 8}" x2="{tick_x:.1f}" y2="{axis_y}" class="grid"/>'
        )
        parts.append(
            f'<text x="{tick_x:.1f}" y="{height - 8}" class="value" text-anchor="middle">{time_text(tick_value)}</text>'
        )

    y_pos = top
    for heading, rows in groups:
        if heading:
            parts.append(
                f'<text x="24" y="{y_pos + 18}" class="heading">{html.escape(heading)}</text>'
            )
            y_pos += heading_height
        for label, value, color_key in rows:
            bar_width = 0 if scale_max <= 0 else chart_width * value / scale_max
            color = colors.get(color_key, "#64748b")
            parts.append(
                f'<text x="24" y="{y_pos + 16}" class="label">{html.escape(label)}</text>'
            )
            parts.append(
                f'<rect x="{axis_x}" y="{y_pos + 5}" width="{bar_width:.1f}" height="14" rx="2" fill="{color}"/>'
            )
            parts.append(
                f'<text x="{axis_x + chart_width + 12}" y="{y_pos + 16}" class="value">{time_text(value)}</text>'
            )
            y_pos += row_height

    parts.append("</svg>")
    path.write_text("\n".join(parts) + "\n", encoding="utf-8")


def generate_charts(
    read_json: list[Path],
    write_json: list[Path],
    output_dir: Path,
    typed_json: list[Path] | None = None,
) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)

    if read_json:
        read_times = load_median_times(read_json)
        for (dataset_name, contract), groups in collect_read_charts(read_times).items():
            suffix, contract_title = CONTRACTS[contract]
            output = output_dir / f"read_{safe_file_name(dataset_name)}_{suffix}.svg"
            write_svg(
                output,
                f"{contract_title}: {nice_title(dataset_name)}",
                groups,
                READ_COLORS,
            )
        generate_throughput_charts(
            read_times, load_processed_bytes(read_json), "read_", output_dir
        )

    if write_json:
        write_times = load_median_times(write_json)
        for dataset_name, groups in collect_write_dataset_charts(write_times).items():
            output = output_dir / f"write_{safe_file_name(dataset_name)}_document.svg"
            write_svg(
                output,
                f"Document serialization: {nice_title(dataset_name)}",
                groups,
                WRITE_COLORS,
            )
        for scenario, groups in collect_write_charts(write_times).items():
            output = output_dir / f"write_{safe_file_name(scenario)}.svg"
            write_svg(
                output,
                f"Write performance: {nice_title(scenario)}",
                groups,
                WRITE_COLORS,
                min_rows=3,
            )
        generate_throughput_charts(
            write_times, load_processed_bytes(write_json), "write_", output_dir
        )

    if typed_json:
        typed_times = load_median_times(typed_json)
        titles = {"read": "Typed read", "write": "Typed write"}
        for (dataset_name, direction), groups in collect_typed_charts(
            typed_times
        ).items():
            output = output_dir / f"typed_{direction}_{safe_file_name(dataset_name)}.svg"
            write_svg(
                output,
                f"{titles[direction]}: {nice_title(dataset_name)}",
                groups,
                TYPED_COLORS,
            )
        generate_throughput_charts(
            typed_times, load_processed_bytes(typed_json), "typed_", output_dir
        )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--read-json",
        type=Path,
        nargs="+",
        default=[],
        help="Google Benchmark JSON files for read benchmarks",
    )
    parser.add_argument(
        "--write-json",
        type=Path,
        nargs="+",
        default=[],
        help="Google Benchmark JSON files for write benchmarks",
    )
    parser.add_argument(
        "--typed-json",
        type=Path,
        nargs="+",
        default=[],
        help="Google Benchmark JSON files for the typed read/write benchmarks",
    )
    parser.add_argument(
        "--output-dir", type=Path, default=Path("docs/benchmark/images")
    )
    args = parser.parse_args()
    if not args.read_json and not args.write_json and not args.typed_json:
        parser.error(
            "at least one of --read-json, --write-json or --typed-json is required"
        )
    for json_path in [*args.read_json, *args.write_json, *args.typed_json]:
        if not json_path.is_file():
            parser.error(f"JSON file does not exist: {json_path}")
    return args


def main() -> None:
    args = parse_args()
    try:
        generate_charts(
            args.read_json, args.write_json, args.output_dir, args.typed_json
        )
    except ValueError as error:
        raise SystemExit(f"error: {error}") from error


if __name__ == "__main__":
    main()
