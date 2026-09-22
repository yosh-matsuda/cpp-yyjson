#!/usr/bin/env python3
"""Decide which benchmarks need more repetitions to reach a target precision.

Google Benchmark always runs a fixed number of repetitions, which wastes time on the
benchmarks that are already stable and can still be too few for the noisy ones.  This
script reads the result of a run, estimates the relative standard error of the reported
median from the observed coefficient of variation, and prints the benchmarks that have
not reached the target together with the repetition count they need.

Output is one line per repetition count, machine readable for the runner script:

    <repetitions> <google benchmark filter>
"""

from __future__ import annotations

import argparse
import json
import math
from collections import defaultdict
from pathlib import Path

# Standard error of a median of n samples of a normal distribution, relative to the
# standard error of the mean.  Used to turn the observed spread into a precision target.
MEDIAN_EFFICIENCY = 1.2533

# The coefficient of variation is itself estimated from a small sample, so aim slightly
# higher than the requested precision.
SAFETY_FACTOR = 1.25


def collect(path: Path) -> dict[str, tuple[float, int]]:
    """Return {benchmark name: (coefficient of variation, repetitions)}."""
    with path.open(encoding="utf-8") as handle:
        data = json.load(handle)

    result: dict[str, tuple[float, int]] = {}
    for row in data.get("benchmarks", []):
        name = row.get("name")
        if not isinstance(name, str) or row.get("aggregate_name") != "cv":
            continue
        repetitions = row.get("iterations")
        variation = row.get("real_time")
        if not isinstance(repetitions, int) or not isinstance(variation, int | float):
            continue
        result[name.removesuffix("_cv")] = (float(variation), repetitions)
    return result


def required_repetitions(variation: float, target: float) -> int:
    """Repetitions needed so that the relative standard error of the median <= target."""
    if variation <= 0.0:
        return 1
    return math.ceil((MEDIAN_EFFICIENCY * variation * SAFETY_FACTOR / target) ** 2)


def normalize_tiers(tiers: list[int], max_repetitions: int) -> list[int]:
    return sorted({tier for tier in [*tiers, max_repetitions] if tier <= max_repetitions})


def choose_tier(needed: int, repetitions: int, tiers: list[int]) -> int | None:
    """Smallest tier that covers the need, None when enough repetitions were measured."""
    if needed <= repetitions:
        return None
    candidates = [tier for tier in tiers if tier > repetitions]
    if not candidates:
        return None
    return next((tier for tier in candidates if tier >= needed), candidates[-1])


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", type=Path, required=True, help="result of the last run")
    parser.add_argument(
        "--target",
        type=float,
        default=0.005,
        help="relative standard error of the median to reach (default: 0.005 = 0.5%%)",
    )
    parser.add_argument(
        "--max-repetitions",
        type=int,
        default=101,
        help="never ask for more repetitions than this (default: 101)",
    )
    parser.add_argument(
        "--tiers",
        type=int,
        nargs="+",
        default=[21, 51],
        help="repetition counts to escalate through (default: 21 51)",
    )
    parser.add_argument(
        "--report",
        action="store_true",
        help="describe the benchmarks that stay above the target instead of the plan",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    measured = collect(args.json)

    tiers = normalize_tiers(args.tiers, args.max_repetitions)

    groups: dict[int, list[str]] = defaultdict(list)
    unreachable: list[tuple[str, float, int]] = []
    for name, (variation, repetitions) in sorted(measured.items()):
        needed = required_repetitions(variation, args.target)
        if needed <= repetitions:
            continue
        tier = choose_tier(needed, repetitions, tiers)
        if tier is None:
            unreachable.append((name, variation, repetitions))
            continue
        groups[tier].append(name)

    if args.report:
        for name, variation, repetitions in sorted(
            unreachable, key=lambda item: item[1], reverse=True
        ):
            achieved = MEDIAN_EFFICIENCY * variation / math.sqrt(repetitions)
            print(f"{name}: {achieved * 100:.2f}% at {repetitions} repetitions")
        return

    for repetitions, names in sorted(groups.items()):
        pattern = "^(" + "|".join(names) + ")$"
        print(f"{repetitions} {pattern}")


if __name__ == "__main__":
    main()
