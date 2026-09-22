#!/usr/bin/env python3
"""Merge a partial Google Benchmark run into the stored benchmark results.

A filtered re-run only measures a subset of the benchmarks, so its output must be
merged into the committed result files instead of replacing them.  Rows are keyed
by benchmark name: rows measured by the new run replace the stored ones, all other
rows are carried over unchanged.  Both the JSON output (``--benchmark_out``) and the
console log are supported; the format is selected from the target file extension.
"""

from __future__ import annotations

import argparse
import json
import shutil
from pathlib import Path

MEASURED_AT_KEY = "cpp_yyjson_measured_at"
AGGREGATE_SUFFIXES = ("_mean", "_median", "_stddev", "_cv")


def die(message: str) -> None:
    raise SystemExit(f"error: {message}")


def dump_json(value: object, indent: int = 0) -> str:
    """Serialize like Google Benchmark does, so untouched rows stay byte-identical."""
    pad = "  " * indent
    inner_pad = "  " * (indent + 1)
    if isinstance(value, dict):
        if not value:
            return "{}"
        items = ",\n".join(
            f"{inner_pad}{json.dumps(str(key))}: {dump_json(item, indent + 1)}"
            for key, item in value.items()
        )
        return f"{{\n{items}\n{pad}}}"
    if isinstance(value, list):
        if not value:
            return "[]"
        if not any(isinstance(item, dict | list) for item in value):
            return "[" + ",".join(dump_json(item, indent) for item in value) + "]"
        items = ",\n".join(f"{inner_pad}{dump_json(item, indent + 1)}" for item in value)
        return f"[\n{items}\n{pad}]"
    if isinstance(value, bool) or value is None or isinstance(value, int):
        return json.dumps(value)
    if isinstance(value, float):
        return f"{value:.16e}"
    return json.dumps(value)


def run_date(context: dict) -> str:
    date = context.get("date")
    return date if isinstance(date, str) else "unknown"


def annotate(benchmarks: list[dict], date: str) -> None:
    for row in benchmarks:
        row.setdefault(MEASURED_AT_KEY, date)


def median_values(benchmarks: list[dict]) -> dict[str, float]:
    values: dict[str, float] = {}
    for row in benchmarks:
        name = row.get("name")
        time = row.get("real_time")
        if not isinstance(name, str) or not isinstance(time, int | float):
            continue
        if row.get("aggregate_name") == "median" or name.endswith("_median"):
            values[name] = float(time)
    return values


def report_changes(old: list[dict], new: list[dict], quiet: bool = False) -> None:
    old_values = median_values(old)
    new_values = median_values(new)
    shared = [name for name in new_values if name in old_values]
    if not shared:
        return

    def change(name: str) -> float:
        before = old_values[name]
        return 0.0 if before == 0 else (new_values[name] - before) / before * 100.0

    shared.sort(key=lambda name: abs(change(name)), reverse=True)
    if quiet:
        worst = shared[0]
        print(
            f"  worst median change: {worst.removesuffix('_median')} ({change(worst):+.1f}%)"
        )
        return
    print(f"  median comparison ({len(shared)} benchmarks, worst deviations first):")
    for name in shared[:5]:
        label = name.removesuffix("_median")
        print(
            f"    {label}: {old_values[name]:.4g} -> {new_values[name]:.4g}"
            f" ({change(name):+.1f}%)"
        )
    outliers = sum(1 for name in shared if abs(change(name)) >= 5.0)
    if outliers:
        print(f"    {outliers} benchmark(s) deviate by 5% or more from the stored run")


def merge_json(target: Path, update: Path, quiet: bool = False) -> None:
    stored = json.loads(target.read_text(encoding="utf-8"))
    fresh = json.loads(update.read_text(encoding="utf-8"))

    stored_rows: list[dict] = stored.get("benchmarks", [])
    fresh_rows: list[dict] = fresh.get("benchmarks", [])
    if not fresh_rows:
        die(f"no benchmark rows in {update}")

    annotate(stored_rows, run_date(stored.get("context", {})))
    annotate(fresh_rows, run_date(fresh.get("context", {})))

    fresh_by_name = {row["name"]: row for row in fresh_rows if "name" in row}
    merged: list[dict] = []
    seen: set[str] = set()
    for row in stored_rows:
        name = row.get("name")
        if isinstance(name, str) and name in fresh_by_name:
            merged.append(fresh_by_name[name])
            seen.add(name)
        else:
            merged.append(row)
    added = [row for name, row in fresh_by_name.items() if name not in seen]
    merged.extend(added)

    report_changes(stored_rows, fresh_rows, quiet)
    if added:
        print(f"  added {len(added)} benchmark(s) that were not in the stored results")
    carried = len(merged) - len(fresh_by_name)
    print(f"  {len(fresh_by_name)} row(s) updated, {carried} row(s) carried over")

    stored["context"] = fresh.get("context", stored.get("context", {}))
    stored["benchmarks"] = merged
    target.write_text(dump_json(stored) + "\n", encoding="utf-8")


def split_log(path: Path) -> tuple[str, list[tuple[str, str]]]:
    """Return the column header and the (name, columns) pairs of a benchmark log.

    Google Benchmark left-justifies the name in a field as wide as the longest
    benchmark name of the run, so the column part of every line starts right after
    that field and stays self-consistent even when a filtered run has shorter names.
    """
    lines = path.read_text(encoding="utf-8").splitlines()
    data_start = 0
    for index, line in enumerate(lines):
        stripped = line.strip()
        if not stripped or set(stripped) == {"-"} or stripped.startswith("Benchmark"):
            data_start = index + 1
            continue
        break
    header = lines[:data_start]
    rows = [line for line in lines[data_start:] if line.strip()]
    if not rows:
        die(f"no benchmark rows in {path}")

    name_width = max(len(line.split(maxsplit=1)[0]) for line in rows)
    entries: list[tuple[str, str]] = []
    for line in rows:
        name = line.split(maxsplit=1)[0]
        if line[len(name) : name_width].strip():
            die(f"unexpected column layout in {path}: {line!r}")
        entries.append((name, line[name_width:]))

    columns_header = ""
    for line in header:
        if line.startswith("Benchmark"):
            columns_header = line[name_width:]
            break
    return columns_header, entries


def merge_log(target: Path, update: Path) -> None:
    stored_columns, stored_rows = split_log(target)
    fresh_columns, fresh_rows = split_log(update)

    fresh_by_name = dict(fresh_rows)
    merged: list[tuple[str, str]] = []
    seen: set[str] = set()
    for name, columns in stored_rows:
        if name in fresh_by_name:
            merged.append((name, fresh_by_name[name]))
            seen.add(name)
        else:
            merged.append((name, columns))
    added = [(name, columns) for name, columns in fresh_rows if name not in seen]
    merged.extend(added)

    columns_header = max((stored_columns, fresh_columns), key=len)
    name_width = max(len(name) for name, _ in merged)
    header_line = "Benchmark".ljust(name_width) + columns_header
    rule = "-" * len(header_line)

    lines = [rule, header_line, rule]
    lines.extend(name.ljust(name_width) + columns for name, columns in merged)
    target.write_text("\n".join(lines) + "\n", encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--target", type=Path, required=True, help="stored result file to update"
    )
    parser.add_argument(
        "--update", type=Path, required=True, help="result file of the new run"
    )
    parser.add_argument(
        "--quiet", action="store_true", help="summarize instead of listing every change"
    )
    parser.add_argument(
        "--compare-only",
        action="store_true",
        help="report how the new run deviates from the stored one without writing",
    )
    args = parser.parse_args()
    if not args.update.is_file():
        parser.error(f"update file does not exist: {args.update}")
    if args.target.suffix not in {".json", ".log"}:
        parser.error(f"unsupported target format: {args.target}")
    if args.target.suffix != args.update.suffix:
        parser.error("target and update must have the same format")
    return args


def compare_json(target: Path, update: Path) -> None:
    stored = json.loads(target.read_text(encoding="utf-8")).get("benchmarks", [])
    fresh = json.loads(update.read_text(encoding="utf-8")).get("benchmarks", [])
    report_changes(stored, fresh)


def main() -> None:
    args = parse_args()
    if args.compare_only:
        if args.target.is_file() and args.target.suffix == ".json":
            compare_json(args.target, args.update)
        return
    if not args.target.is_file():
        args.target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(args.update, args.target)
        print(f"  created {args.target} from the new run")
        return
    if args.target.suffix == ".json":
        merge_json(args.target, args.update, args.quiet)
    else:
        merge_log(args.target, args.update)


if __name__ == "__main__":
    main()
