#!/usr/bin/env python3
"""Generate README benchmark charts from Google Benchmark JSON output."""

from __future__ import annotations

import argparse
import html
import json
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
}


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
    if name.startswith("write_rapidjson_"):
        return name.removeprefix("write_rapidjson_"), "rapidjson"
    if name.startswith("write_nlohmann_"):
        return name.removeprefix("write_nlohmann_"), "nlohmann-json"
    return None


def collect_write_charts(
    times: dict[str, float],
) -> dict[str, list[tuple[str, list[tuple[str, float, str]]]]]:
    scenarios: dict[str, list[tuple[str, float, str]]] = defaultdict(list)
    unknown_names: set[str] = set()

    for name, value in times.items():
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
    read_json: list[Path], write_json: list[Path], output_dir: Path
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

    if write_json:
        write_times = load_median_times(write_json)
        for scenario, groups in collect_write_charts(write_times).items():
            output = output_dir / f"write_{safe_file_name(scenario)}.svg"
            write_svg(
                output,
                f"Write performance: {nice_title(scenario)}",
                groups,
                WRITE_COLORS,
                min_rows=4,
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
        "--output-dir", type=Path, default=Path("docs/benchmark/images")
    )
    args = parser.parse_args()
    if not args.read_json and not args.write_json:
        parser.error("at least one of --read-json or --write-json is required")
    for json_path in [*args.read_json, *args.write_json]:
        if not json_path.is_file():
            parser.error(f"JSON file does not exist: {json_path}")
    return args


def main() -> None:
    args = parse_args()
    try:
        generate_charts(args.read_json, args.write_json, args.output_dir)
    except ValueError as error:
        raise SystemExit(f"error: {error}") from error


if __name__ == "__main__":
    main()
