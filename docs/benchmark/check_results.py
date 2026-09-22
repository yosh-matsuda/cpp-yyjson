#!/usr/bin/env python3
"""Check the cpp-yyjson rows against the upstream yyjson rows they are compared to.

cpp-yyjson wraps the same backend, so a scenario in which it is slower than the raw C API
is a finding: a regression in the C++ layer, or an artifact of how the benchmark or the
backend is built.  The two sides come from different executables, so Google Benchmark's
own statistics cannot catch it.  This check pairs every `*_cpp_yyjson_*` row of the merged
results with its `*_c_yyjson_*` counterpart and reports the ones that lost by more than the
tolerance, which absorbs run-to-run spread, not a standing handicap.

Before blaming the C++ layer for a loss, check what the loop spends its time on and
rebuild the benchmark against the packaged yyjson; with both sides on the same library the
wrapper has matched the C API on every row so far.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

UNITS = {"ns": 1e-9, "us": 1e-6, "ms": 1e-3, "s": 1.0}
# Markers that name a cpp-yyjson-only variant of a scenario the C API also implements.
# They appear at the end of the benchmark name (`..._array_object_reflection`) as well as
# in the middle of it (`..._single_array_int64`).
VARIANT_MARKERS = ("_reflection", "_macro", "_range", "_single")


def medians(paths: list[Path]) -> dict[str, float]:
    values: dict[str, float] = {}
    for path in paths:
        data = json.loads(path.read_text(encoding="utf-8"))
        for row in data.get("benchmarks", []):
            name = row.get("name", "")
            if row.get("aggregate_name") != "median" and not name.endswith("_median"):
                continue
            unit = UNITS.get(row.get("time_unit"), 1e-3)
            values[name.removesuffix("_median")] = float(row["real_time"]) * unit
    return values


def counterparts(name: str) -> list[str]:
    """The upstream names `name` may be compared against, most specific first.

    Google Benchmark appends the arguments and `manual_time` to the registered name, so the
    variant markers have to be looked for in the name itself rather than in the whole row.
    """
    head, separator, arguments = name.partition("/")
    head = head.replace("_cpp_yyjson", "_c_yyjson", 1)
    candidates = [head]
    for marker in VARIANT_MARKERS:
        stripped = [
            candidate.removesuffix(marker) if candidate.endswith(marker)
            else candidate.replace(marker, "", 1)
            for candidate in candidates
            if marker in candidate
        ]
        candidates.extend(stripped)
    return [candidate + separator + arguments for candidate in candidates]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", type=Path, nargs="+", required=True)
    parser.add_argument(
        "--tolerance",
        type=float,
        default=2.0,
        help="report a scenario when cpp-yyjson is slower by more than this %% (default: 2)",
    )
    args = parser.parse_args()

    values = medians(args.json)
    limit = 1.0 + args.tolerance / 100.0

    losses: list[tuple[str, float, float, float]] = []
    unpaired: list[str] = []
    compared = 0
    for name, time in sorted(values.items()):
        if "_cpp_yyjson" not in name:
            continue
        reference = next(
            (values[other] for other in counterparts(name) if values.get(other, 0.0) > 0.0),
            None,
        )
        if reference is None:
            unpaired.append(name)
            continue
        compared += 1
        ratio = time / reference
        if ratio > limit:
            losses.append((name, time, reference, ratio))

    if not compared:
        print("  no cpp-yyjson/yyjson pair found; nothing to check")
        return
    # An unpaired row is a hole in this check, not a pass.
    for name in unpaired:
        print(f"  no yyjson counterpart for {name}; not checked")
    if not losses:
        print(f"  cpp-yyjson is at most {args.tolerance:.0f}% slower than yyjson "
              f"in all {compared} compared scenarios")
        return

    print(f"  cpp-yyjson is slower than yyjson in {len(losses)} of {compared} scenarios:")
    for name, time, reference, ratio in sorted(losses, key=lambda row: -row[3]):
        label = re.sub(r"^(read|write)_cpp_yyjson_?", "", name)
        print(f"    {label}: {time * 1e3:.3g} ms vs {reference * 1e3:.3g} ms ({ratio:.3f}x)")
    raise SystemExit(1)


if __name__ == "__main__":
    main()
