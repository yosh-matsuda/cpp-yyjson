#!/usr/bin/env bash
#
# Re-measure the benchmarks shown in docs/benchmark/README.md and refresh the results.
#
# The script is meant to be run by a human on an idle machine: interactive agents,
# editors and background services distort the numbers.  It runs the Google Benchmark
# executables from the repository root (the datasets are referenced by relative path),
# merges the results into test/*.json and test/*.log and regenerates the SVG charts.
#
# Run with --help for the scopes, the environment switches and the defaults.

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd -- "${SCRIPT_DIR}/../.." && pwd)
cd -- "${REPO_ROOT}"

SCOPE=cpp
FILTER=
FILTER_SET=0
MIN_REPETITIONS=11
MAX_REPETITIONS=101
TIERS="21 51"
TARGET_PRECISION=0.5
ADAPTIVE=1
ISOLATE=1
BUILD_DIR=build
CONFIG=Release
RESULT_DIR=test
DOC=docs/benchmark/README.md
IMAGE_DIR=docs/benchmark/images
DO_BUILD=0
DO_PLOT=1
PREFLIGHT_ONLY=0
ALLOW_BUSY=0
TUNE_GOVERNOR=0
TUNE_TURBO=0
HIGH_PRIORITY=0
KEEP_RAW=

CPP_READ=cpp_yyjson_bench_read
CPP_WRITE=cpp_yyjson_bench_write
C_READ=yyjson_bench_read
C_WRITE=yyjson_bench_write

COMPETITOR_FILTER='^(read|write)_(rapidjson|simdjson|nlohmann|c_yyjson)'

info() { printf '\033[1m==>\033[0m %s\n' "$*"; }
warn() { printf '\033[33mwarning:\033[0m %s\n' "$*" >&2; }
fail() { printf '\033[31merror:\033[0m %s\n' "$*" >&2; exit 1; }

GOVERNOR_RESTORE=
TURBO_RESTORE=
RAW_DIR=
SUDO_KEEPALIVE_PID=

# The measurement outlasts sudo's credential cache, so a credential obtained at the start
# would expire before the settings have to be restored.  Authenticate once and keep the session
# alive for as long as the script runs.
start_sudo_session() {
    if [[ -n ${SUDO_KEEPALIVE_PID} ]]; then
        return 0
    fi
    info "The selected options need sudo; authenticating once for the whole run"
    sudo -v || fail "sudo authentication failed"
    local parent=$$
    while true; do
        kill -0 "${parent}" 2>/dev/null || break
        sudo -n true 2>/dev/null || break
        sleep 50
    done &
    SUDO_KEEPALIVE_PID=$!
}

on_exit() {
    if [[ -n ${GOVERNOR_RESTORE} ]]; then
        info "Restoring the CPU governor to ${GOVERNOR_RESTORE}"
        sudo sh -c "for f in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do echo ${GOVERNOR_RESTORE} > \"\$f\"; done"
        GOVERNOR_RESTORE=
    fi
    if [[ -n ${TURBO_RESTORE} ]]; then
        info "Restoring intel_pstate/no_turbo to ${TURBO_RESTORE}"
        sudo sh -c "echo ${TURBO_RESTORE} > /sys/devices/system/cpu/intel_pstate/no_turbo"
        TURBO_RESTORE=
    fi
    if [[ -n ${RAW_DIR} ]]; then
        if [[ -n ${KEEP_RAW} ]]; then
            mkdir -p -- "${KEEP_RAW}"
            cp -a -- "${RAW_DIR}"/. "${KEEP_RAW}/" 2>/dev/null || true
            info "Raw results of this run kept in ${KEEP_RAW}"
        fi
        rm -rf -- "${RAW_DIR}"
        RAW_DIR=
    fi
    if [[ -n ${SUDO_KEEPALIVE_PID} ]]; then
        kill "${SUDO_KEEPALIVE_PID}" 2>/dev/null || true
        SUDO_KEEPALIVE_PID=
    fi
}
trap on_exit EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

usage() {
    cat <<'EOF'
Usage: docs/benchmark/run_benchmarks.sh [options]

Scope (which benchmarks are measured):
  --scope cpp            cpp-yyjson rows only, read and write (default, ~15 min)
  --scope cpp-read       cpp-yyjson read rows only (~9 min)
  --scope cpp-write      cpp-yyjson write rows only (~6 min)
  --scope competitors    rapidjson/simdjson/nlohmann-json/yyjson rows only (~40 min)
  --scope read           every read benchmark, both executables (~40 min)
  --scope write          every write benchmark, both executables (~15 min)
  --scope all            everything (~55 min)
  --filter REGEX         Google Benchmark filter, overrides the scope default

Repetitions are chosen from the measured spread: every benchmark starts at
--min-repetitions and is re-measured with more repetitions until the relative standard
error of its median drops below --target-precision.

  --min-repetitions N    repetitions every benchmark gets (default: 11)
  --max-repetitions N    repetitions a noisy benchmark may escalate to (default: 101)
  --target-precision P   relative standard error of the median in % (default: 0.5)
  --repetitions N        use N repetitions for every benchmark, without escalation
  --quick                shorthand for --repetitions 5, for sanity checks only

Every benchmark runs in its own process, so that a benchmark is never measured in
the allocator state left behind by the ones before it.  The allocation-heavy rows
are sensitive to that state by far more than the spread within a single run, and
the cpp-yyjson and yyjson rows come from different executables, so without this the
two sides of a comparison would be measured under different conditions.

  --no-isolate           measure every benchmark of an executable in one process

Environment:
  --tune-governor        set the CPU governor to performance and restore it (sudo)
  --tune-turbo           disable turbo boost while measuring and restore it (sudo)
  --high-priority        run the executables with nice -n -20 (sudo)
  --allow-busy           run even if the machine is busy or misconfigured

Other:
  --build                build the benchmark targets before measuring
  --build-dir DIR        CMake build directory (default: build)
  --config NAME          CMake configuration (default: Release)
  --result-dir DIR       directory holding the stored results (default: test)
  --image-dir DIR        directory for the charts (default: docs/benchmark/images)
  --keep-raw DIR         keep the unmerged output of this run in DIR
  --no-plot              do not regenerate docs/benchmark/images
  --preflight            only run the environment checks
  -h, --help             show this help

Results of a filtered run are merged into the stored files, so unmeasured rows keep
their previous values.  A run without a filter replaces the stored files instead.
EOF
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --scope) SCOPE=$2; shift 2 ;;
        --filter) FILTER=$2; FILTER_SET=1; shift 2 ;;
        --min-repetitions) MIN_REPETITIONS=$2; shift 2 ;;
        --max-repetitions) MAX_REPETITIONS=$2; shift 2 ;;
        --target-precision) TARGET_PRECISION=$2; shift 2 ;;
        --repetitions) MIN_REPETITIONS=$2; MAX_REPETITIONS=$2; ADAPTIVE=0; shift 2 ;;
        --quick) MIN_REPETITIONS=5; MAX_REPETITIONS=5; ADAPTIVE=0; shift ;;
        --no-isolate) ISOLATE=0; shift ;;
        --build) DO_BUILD=1; shift ;;
        --build-dir) BUILD_DIR=$2; shift 2 ;;
        --config) CONFIG=$2; shift 2 ;;
        --result-dir) RESULT_DIR=$2; shift 2 ;;
        --image-dir) IMAGE_DIR=$2; shift 2 ;;
        --keep-raw) KEEP_RAW=$2; shift 2 ;;
        --no-plot) DO_PLOT=0; shift ;;
        --preflight) PREFLIGHT_ONLY=1; shift ;;
        --allow-busy) ALLOW_BUSY=1; shift ;;
        --tune-governor) TUNE_GOVERNOR=1; shift ;;
        --tune-turbo) TUNE_TURBO=1; shift ;;
        --high-priority) HIGH_PRIORITY=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) usage >&2; fail "unknown option: $1" ;;
    esac
done

TARGETS=()
case ${SCOPE} in
    cpp)
        TARGETS=("${CPP_READ}" "${CPP_WRITE}")
        [[ ${FILTER_SET} -eq 1 ]] || FILTER='^(read|write)_cpp_yyjson'
        ;;
    cpp-read)
        TARGETS=("${CPP_READ}")
        [[ ${FILTER_SET} -eq 1 ]] || FILTER='^read_cpp_yyjson'
        ;;
    cpp-write)
        TARGETS=("${CPP_WRITE}")
        [[ ${FILTER_SET} -eq 1 ]] || FILTER='^write_cpp_yyjson'
        ;;
    competitors)
        TARGETS=("${CPP_READ}" "${CPP_WRITE}" "${C_READ}" "${C_WRITE}")
        [[ ${FILTER_SET} -eq 1 ]] || FILTER=${COMPETITOR_FILTER}
        ;;
    read) TARGETS=("${CPP_READ}" "${C_READ}") ;;
    write) TARGETS=("${CPP_WRITE}" "${C_WRITE}") ;;
    all) TARGETS=("${CPP_READ}" "${CPP_WRITE}" "${C_READ}" "${C_WRITE}") ;;
    *) fail "unknown scope: ${SCOPE}" ;;
esac

[[ ${MIN_REPETITIONS} =~ ^[0-9]+$ && ${MIN_REPETITIONS} -ge 1 ]] || fail "invalid repetitions: ${MIN_REPETITIONS}"
[[ ${MAX_REPETITIONS} =~ ^[0-9]+$ && ${MAX_REPETITIONS} -ge ${MIN_REPETITIONS} ]] || fail "invalid maximum repetitions: ${MAX_REPETITIONS}"
[[ ${TARGET_PRECISION} =~ ^[0-9]*\.?[0-9]+$ ]] || fail "invalid target precision: ${TARGET_PRECISION}"

BIN_DIR="${BUILD_DIR}/test/${CONFIG}"
[[ -d ${BIN_DIR} ]] || BIN_DIR="${BUILD_DIR}/test"

#
# PREFLIGHT
#
preflight_failed=0
preflight_error() {
    if [[ ${ALLOW_BUSY} -eq 1 ]]; then
        warn "$1 (ignored because of --allow-busy)"
    else
        printf '\033[31mfailed check:\033[0m %s\n' "$1" >&2
        preflight_failed=1
    fi
}

info "Preflight checks"

dataset_dir=external/yyjson_benchmark/data/json
dataset_count=$(find "${dataset_dir}" -maxdepth 1 -name '*.json' 2>/dev/null | wc -l)
if [[ ${dataset_count} -lt 10 ]]; then
    fail "benchmark datasets are missing; run: git submodule update --init --recursive"
fi
printf '  datasets            : %s files in %s\n' "${dataset_count}" "${dataset_dir}"

for target in "${TARGETS[@]}"; do
    if [[ ! -x ${BIN_DIR}/${target} ]] && [[ ${DO_BUILD} -eq 0 ]]; then
        fail "missing executable ${BIN_DIR}/${target}; configure with -DCPPYYJSON_BUILD_BENCH=ON and rerun with --build"
    fi
done

if [[ ${DO_BUILD} -eq 1 ]]; then
    info "Building ${TARGETS[*]}"
    cmake --build "${BUILD_DIR}" --config "${CONFIG}" --target "${TARGETS[@]}"
else
    # Which sources reach which executable is something the build system already
    # worked out while compiling, and a benchmark that only exists in the read
    # program must not make the write program look out of date.  Fall back to the
    # whole tree when the generator keeps its dependencies somewhere else: that
    # only ever asks for a rebuild that was not needed.
    stale=$(python3 - "${PWD}" "${BUILD_DIR}" "${BIN_DIR}" "${TARGETS[@]}" <<'PY'
import glob
import os
import sys

root, build, bindir = (os.path.realpath(p) for p in sys.argv[1:4])
targets = sys.argv[4:]


def target_dir(name):
    found = glob.glob(os.path.join(build, "**", "CMakeFiles", name + ".dir"), recursive=True)
    return found[0] if found else None


def listed_in(depfile):
    try:
        with open(depfile, encoding="utf-8", errors="replace") as handle:
            text = handle.read()
    except OSError:
        return
    for line in text.replace("\\\n", " ").splitlines():
        _, _, deps = line.partition(":")
        yield from deps.split()


def resolve(directory, dep):
    if os.path.isabs(dep):
        return os.path.realpath(dep)
    # A depfile written by the compiler names its dependencies relative to the
    # directory the rule ran in, which is the one holding the `CMakeFiles` tree
    # rather than the `.dir` the file itself sits in.
    for base in (os.path.dirname(os.path.dirname(directory)), build, directory):
        candidate = os.path.realpath(os.path.join(base, dep))
        if os.path.exists(candidate):
            return candidate
    return os.path.realpath(os.path.join(directory, dep))


def sources_of(name, seen):
    directory = target_dir(name)
    if directory is None or name in seen:
        return
    seen.add(name)
    for depfile in glob.glob(os.path.join(directory, "**", "*.d"), recursive=True):
        for dep in listed_in(depfile):
            path = resolve(directory, dep)
            if path.startswith(build + os.sep):
                # A static library of this project stands for the sources behind it.
                stem = os.path.basename(path)
                if stem.startswith("lib") and stem.endswith(".a"):
                    yield from sources_of(stem[3:-2], seen)
            elif path.startswith(root + os.sep):
                yield path


checked = 0
for target in targets:
    binary = os.path.join(bindir, target)
    if not os.path.exists(binary):
        continue
    built = os.path.getmtime(binary)
    seen = set()
    for source in sources_of(target, seen):
        checked += 1
        if os.path.getmtime(source) > built:
            print(target, os.path.relpath(source, root))
            raise SystemExit
if checked == 0:
    print("?")
PY
    )
    if [[ ${stale} == "?" ]]; then
        newest_source=$(find include src test CMakeLists.txt vcpkg.json -newer "${BIN_DIR}/${TARGETS[0]}" \
            \( -name '*.cpp' -o -name '*.hpp' -o -name 'CMakeLists.txt' -o -name 'vcpkg.json' \) 2>/dev/null | head -n 1)
        [[ -z ${newest_source} ]] || stale="${TARGETS[0]} ${newest_source}"
    fi
    if [[ -n ${stale} && ${stale} != "?" ]]; then
        read -r stale_target stale_source <<<"${stale}"
        preflight_error "${stale_target} is older than ${stale_source}; rerun with --build"
    fi
fi

if command -v readelf >/dev/null 2>&1; then
    binary_gcc=$(readelf -p .comment "${BIN_DIR}/${TARGETS[0]}" 2>/dev/null |
        grep -oE 'GCC: \([^)]*\) [0-9.]+' | sed 's/.*) //' | sort -u | paste -sd, -)
    # `grep` finding nothing is not an error here, but it would end the script
    # under `set -o pipefail`, so the pipeline is allowed to come back empty.
    readme_gcc=$(grep -ohE 'compiled with GCC [0-9]+(\.[0-9]+){0,2}' "${DOC}" 2>/dev/null |
        head -n 1 | sed 's/.*GCC //' || true)
    printf '  compiler (binary)   : GCC %s\n' "${binary_gcc:-unknown}"
    printf '  compiler (README)   : GCC %s\n' "${readme_gcc:-unknown}"
    if command -v g++ >/dev/null 2>&1; then
        printf '  compiler (default)  : GCC %s\n' "$(g++ -dumpfullversion 2>/dev/null || echo unknown)"
    fi
    if [[ -n ${binary_gcc} && -n ${readme_gcc} && ${binary_gcc} != "${readme_gcc}" ]]; then
        if [[ -n ${FILTER} ]]; then
            preflight_error "the executables were built with GCC ${binary_gcc} but ${DOC} documents GCC ${readme_gcc}; merging a partial run would mix toolchains, use --scope all and update ${DOC}"
        else
            warn "the executables were built with GCC ${binary_gcc} but ${DOC} documents GCC ${readme_gcc}; update its environment paragraph after this run"
        fi
    fi
fi

# The bundled backend selects its AVX2 paths from `__AVX2__` at compile time, and
# `CPPYYJSON_ENABLE_AVX2` only defaults to ON when the benchmarks are configured in
# the same run.  Configuring the project without `-DCPPYYJSON_BUILD_BENCH=ON` first
# leaves the option cached OFF, and every later run then measures the SSE2 paths
# without saying so, which is worth more than 10% on the string-heavy rows.  Read
# the answer off the archive rather than the cache, so that a stale build is caught
# as well.
backend_archive=$(find "${BUILD_DIR}" -name 'libcpp_yyjson.a' -print -quit 2>/dev/null)
if [[ -n ${backend_archive} ]] && command -v objdump >/dev/null 2>&1; then
    # `grep -q` would close the pipe early and trip `pipefail`, so count instead.
    backend_ymm=$(objdump -d "${backend_archive}" 2>/dev/null | grep -c '%ymm' || true)
    if [[ ${backend_ymm} -gt 0 ]]; then
        printf '  backend simd        : AVX2\n'
    else
        printf '  backend simd        : SSE2 only\n'
        preflight_error "the bundled backend holds no AVX2 code; reconfigure with -DCPPYYJSON_ENABLE_AVX2=ON and rerun with --build"
    fi
fi

# The checks from here on do not only report: they set the governor and turbo boost
# as they go.  Give up before touching the machine when the run is already known to
# be unusable, so that a rebuild is not paid for with a sudo prompt and a settings
# round trip.
if [[ ${preflight_failed} -eq 1 ]]; then
    fail "preflight checks failed; fix the items above or rerun with --allow-busy"
fi

if [[ ${TUNE_GOVERNOR} -eq 1 || ${TUNE_TURBO} -eq 1 || ${HIGH_PRIORITY} -eq 1 ]]; then
    start_sudo_session
fi

governor=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo unknown)
printf '  cpu governor        : %s\n' "${governor}"
if [[ ${governor} != performance ]]; then
    if [[ ${TUNE_GOVERNOR} -eq 1 ]]; then
        info "Setting the CPU governor to performance (sudo)"
        sudo sh -c 'for f in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do echo performance > "$f"; done'
        GOVERNOR_RESTORE=${governor}
    else
        preflight_error "cpu governor is '${governor}', the stored results were measured with 'performance'; add --tune-governor"
    fi
fi

if [[ -r /sys/devices/system/cpu/intel_pstate/no_turbo ]]; then
    no_turbo=$(cat /sys/devices/system/cpu/intel_pstate/no_turbo)
    if [[ ${no_turbo} != 1 && ${TUNE_TURBO} -eq 1 ]]; then
        info "Disabling turbo boost (sudo)"
        sudo sh -c 'echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo'
        TURBO_RESTORE=${no_turbo}
        no_turbo=1
    fi
    printf '  turbo boost         : %s\n' "$([[ ${no_turbo} == 1 ]] && echo disabled || echo enabled)"
fi

max_mhz=unknown
if [[ -r /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq ]]; then
    max_mhz=$(($(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq) / 1000))
    printf '  max frequency       : %s MHz\n' "${max_mhz}"
fi

if [[ -r /sys/devices/system/cpu/smt/control ]]; then
    printf '  smt                 : %s\n' "$(cat /sys/devices/system/cpu/smt/control)"
fi
printf '  online cpus         : %s\n' "$(nproc)"

busy=$(python3 - <<'PY'
import time


def sample():
    with open("/proc/stat", encoding="utf-8") as handle:
        fields = [float(value) for value in handle.readline().split()[1:]]
    return sum(fields), fields[3]


total_before, idle_before = sample()
time.sleep(1.0)
total_after, idle_after = sample()
total = total_after - total_before
idle = idle_after - idle_before
print(f"{0.0 if total <= 0 else (1.0 - idle / total) * 100.0:.1f}")
PY
)
printf '  cpu utilization     : %s%% (sampled over 1 s)\n' "${busy}"
printf '  load average        : %s\n' "$(cut -d' ' -f1-3 /proc/loadavg)"
if awk "BEGIN {exit !(${busy} > 5.0)}"; then
    printf '  busiest processes   :\n'
    ps -eo pcpu,pid,comm --sort=-pcpu --no-headers | head -n 5 | sed 's/^/    /'
    preflight_error "the machine is busy (${busy}% CPU); stop interactive agents, editors and background jobs first"
fi

# Results this run does not overwrite stay in the published set and end up in the
# same charts as the ones it does, so they have to have been measured on the same
# machine at the same frequency.  An executable outside the scope keeps all of its
# rows, and one inside a filtered scope keeps the rows the filter does not select.
surviving=()
for target in "${CPP_READ}" "${CPP_WRITE}" "${C_READ}" "${C_WRITE}"; do
    keeps_rows=1
    if [[ -z ${FILTER} ]]; then
        for measured in "${TARGETS[@]}"; do
            if [[ ${measured} == "${target}" ]]; then
                keeps_rows=0
                break
            fi
        done
    fi
    if [[ ${keeps_rows} -eq 1 && -r ${RESULT_DIR}/${target}.json ]]; then
        surviving+=("${target}")
    fi
done

for target in "${surviving[@]}"; do
    stored_context=$(python3 - "${RESULT_DIR}/${target}.json" <<'PY'
import json
import sys

try:
    with open(sys.argv[1], encoding="utf-8") as handle:
        context = json.load(handle).get("context", {})
except OSError:
    raise SystemExit
# Prefer the cap recorded by stamp_max_frequency; `mhz_per_cpu` is only for older results.
print(context.get("host_name", "?"), context.get("num_cpus", "?"),
      context.get("cpp_yyjson_max_mhz", context.get("mhz_per_cpu", "?")),
      context.get("date", "?"))
PY
    )
    [[ -n ${stored_context} ]] || continue
    read -r stored_host stored_cpus stored_mhz stored_date <<<"${stored_context}"
    printf '  %-20s: %s, %s cpus, %s MHz, %s\n' "kept from ${target}" \
        "${stored_host}" "${stored_cpus}" "${stored_mhz}" "${stored_date}"
    if [[ ${stored_host} != "$(hostname)" || ${stored_cpus} != "$(nproc)" ]]; then
        preflight_error "${target} was measured on ${stored_host} with ${stored_cpus} cpus; merging a partial run into the stored results is only valid on the same machine, use --scope all instead"
    elif [[ ${stored_mhz} != "${max_mhz}" ]]; then
        preflight_error "${target} was measured at ${stored_mhz} MHz but this machine is capped at ${max_mhz} MHz; add --tune-turbo or use --scope all instead"
    fi
done

if [[ ${preflight_failed} -eq 1 ]]; then
    fail "preflight checks failed; fix the items above or rerun with --allow-busy"
fi

estimate() {
    python3 - "${SCRIPT_DIR}" "$1" "$2" <<'PY'
import json
import math
import os
import re
import sys

# Google Benchmark keeps iterating a benchmark until --benchmark_min_time (0.5 s by
# default) is reached, so the cost of a repetition is governed by that floor rather than
# by the time of a single iteration.  The factor covers the iteration count ramp-up.
MIN_TIME = 0.5
OVERHEAD = 1.45
# Starting one process per benchmark costs its setup once more per measurement round.
STARTUP = 0.15

sys.path.insert(0, sys.argv[1])
from select_repetitions import choose_tier, normalize_tiers, required_repetitions

path, pattern = sys.argv[2], sys.argv[3]
minimum = int(os.environ["MIN_REPETITIONS"])
maximum = int(os.environ["MAX_REPETITIONS"])
target = float(os.environ["TARGET_PRECISION"]) / 100.0
adaptive = os.environ["ADAPTIVE"] == "1"
isolate = os.environ["ISOLATE"] == "1"
tiers = normalize_tiers([int(tier) for tier in os.environ["TIERS"].split()], maximum)

units = {"ns": 1e-9, "us": 1e-6, "ms": 1e-3, "s": 1.0}
try:
    with open(path, encoding="utf-8") as handle:
        data = json.load(handle)
except OSError:
    print("0")
    raise SystemExit

matcher = re.compile(pattern) if pattern else None
variation = {
    re.sub(r"_cv$", "", row["name"]): row["real_time"]
    for row in data.get("benchmarks", [])
    if row.get("aggregate_name") == "cv"
}

total = 0.0
for row in data.get("benchmarks", []):
    if row.get("aggregate_name") != "mean":
        continue
    name = re.sub(r"_(mean|median|stddev|cv)$", "", row.get("name", ""))
    if matcher is not None and not matcher.search(name):
        continue
    iteration = row["real_time"] * units.get(row.get("time_unit"), 1e-3)
    if iteration <= 0:
        continue
    per_repetition = iteration * max(1, math.ceil(MIN_TIME / iteration)) * OVERHEAD
    total += minimum * per_repetition + (STARTUP if isolate else 0.0)
    if not adaptive or name not in variation:
        continue
    # The spread of the stored run predicts which benchmarks will be measured again.
    repetitions = minimum
    while (tier := choose_tier(required_repetitions(variation[name], target), repetitions, tiers)) is not None:
        total += tier * per_repetition + (STARTUP if isolate else 0.0)
        repetitions = tier
print(f"{total:.0f}")
PY
}

export MIN_REPETITIONS MAX_REPETITIONS TARGET_PRECISION ADAPTIVE TIERS ISOLATE

total_estimate=0
for target in "${TARGETS[@]}"; do
    seconds=$(estimate "${RESULT_DIR}/${target}.json" "${FILTER}")
    total_estimate=$((total_estimate + seconds))
done

info "Plan"
printf '  executables         : %s\n' "${TARGETS[*]}"
printf '  filter              : %s\n' "${FILTER:-<none, full run>}"
if [[ ${ADAPTIVE} -eq 1 ]]; then
    printf '  repetitions         : %s, up to %s until the median is accurate to %s%%\n' \
        "${MIN_REPETITIONS}" "${MAX_REPETITIONS}" "${TARGET_PRECISION}"
else
    printf '  repetitions         : %s, fixed\n' "${MIN_REPETITIONS}"
fi
printf '  process isolation   : %s\n' "$([[ ${ISOLATE} -eq 1 ]] && echo 'one process per benchmark' || echo 'one process per executable')"
printf '  estimated time      : ~%s h %s min\n' \
    "$((total_estimate / 3600))" "$((total_estimate % 3600 / 60))"

if [[ ${PREFLIGHT_ONLY} -eq 1 ]]; then
    info "Preflight only; nothing was measured"
    exit 0
fi

#
# MEASUREMENT
#
raw_dir=$(mktemp -d)
RAW_DIR=${raw_dir}

wrapper=()
if command -v setarch >/dev/null 2>&1; then
    wrapper=(setarch "$(uname -m)" -R)
else
    warn "setarch is unavailable; the stored results were measured with ASLR disabled"
fi
if [[ ${HIGH_PRIORITY} -eq 1 ]]; then
    wrapper=(sudo nice -n -20 "${wrapper[@]}")
fi

# Google Benchmark records the frequency /proc/cpuinfo reported when the process
# started, which is the idle frequency for an executable that began while the
# machine was quiet.  Record the cap this run actually measured so that a later
# partial run can tell whether it would be merging numbers from another state.
stamp_max_frequency() {
    [[ ${max_mhz} == unknown ]] && return 0
    MAX_MHZ=${max_mhz} python3 - "${SCRIPT_DIR}" "$1" <<'STAMP'
import json
import os
import sys

sys.path.insert(0, sys.argv[1])
from merge_benchmark_results import dump_json

path = sys.argv[2]
with open(path, encoding="utf-8") as handle:
    results = json.load(handle)
results.setdefault("context", {})["cpp_yyjson_max_mhz"] = int(os.environ["MAX_MHZ"])
with open(path, "w", encoding="utf-8") as handle:
    handle.write(dump_json(results) + "\n")
STAMP
}

run_benchmark_process() {
    local target=$1 repetitions=$2 pattern=$3 out_json=$4 out_log=$5
    local args=("./${BIN_DIR}/${target}"
        "--benchmark_repetitions=${repetitions}"
        --benchmark_report_aggregates_only=true
        --benchmark_out_format=json
        "--benchmark_out=${out_json}")
    if [[ -n ${pattern} ]]; then
        args+=("--benchmark_filter=${pattern}")
    fi
    "${wrapper[@]}" "${args[@]}" | tee "${out_log}"
}

list_benchmarks() {
    local target=$1 pattern=$2
    local args=("./${BIN_DIR}/${target}" --benchmark_list_tests=true)
    if [[ -n ${pattern} ]]; then
        args+=("--benchmark_filter=${pattern}")
    fi
    "${args[@]}" 2>/dev/null
}

merge_into() {
    python3 "${SCRIPT_DIR}/merge_benchmark_results.py" --target "$1" --update "$2" --quiet
}

run_benchmark() {
    local target=$1 repetitions=$2 pattern=$3 out_json=$4 out_log=$5
    if [[ ${ISOLATE} -eq 0 ]]; then
        run_benchmark_process "$@"
        return
    fi

    local names=()
    mapfile -t names < <(list_benchmarks "${target}" "${pattern}")
    [[ ${#names[@]} -gt 0 ]] || fail "no benchmark of ${target} matches ${pattern:-<none>}"

    rm -f -- "${out_json}" "${out_log}"
    local index=0 part_json part_log part_err
    for name in "${names[@]}"; do
        index=$((index + 1))
        printf '  [%d/%d] %s\n' "${index}" "${#names[@]}" "${name}"
        part_json="${RAW_DIR}/part.${target}.${index}.json"
        part_log="${RAW_DIR}/part.${target}.${index}.log"
        part_err="${RAW_DIR}/part.${target}.${index}.err"
        # The names are plain identifiers, so anchoring them is enough to select one.
        if ! run_benchmark_process "${target}" "${repetitions}" "^${name}\$" \
            "${part_json}" "${part_log}" >/dev/null 2>"${part_err}"; then
            cat -- "${part_err}" >&2
            fail "${target} failed while measuring ${name}"
        fi
        merge_into "${out_json}" "${part_json}" >/dev/null
        merge_into "${out_log}" "${part_log}" >/dev/null
        rm -f -- "${part_json}" "${part_log}" "${part_err}"
    done
}

select_repetitions() {
    python3 "${SCRIPT_DIR}/select_repetitions.py" \
        --json "$1" \
        --target "$(awk "BEGIN {print ${TARGET_PRECISION} / 100.0}")" \
        --max-repetitions "${MAX_REPETITIONS}" \
        --tiers ${TIERS} \
        "${@:2}"
}

mkdir -p -- "${RESULT_DIR}"
for target in "${TARGETS[@]}"; do
    raw_json="${raw_dir}/${target}.json"
    raw_log="${raw_dir}/${target}.log"

    info "Running ${target} with ${MIN_REPETITIONS} repetitions"
    run_benchmark "${target}" "${MIN_REPETITIONS}" "${FILTER}" "${raw_json}" "${raw_log}"

    if [[ ${ADAPTIVE} -eq 1 ]]; then
        round=0
        while :; do
            round=$((round + 1))
            mapfile -t plan < <(select_repetitions "${raw_json}")
            [[ ${#plan[@]} -gt 0 ]] || break
            for entry in "${plan[@]}"; do
                repetitions=${entry%% *}
                pattern=${entry#* }
                count=$(awk -F'|' '{print NF}' <<<"${pattern}")
                info "Re-measuring ${count} benchmark(s) of ${target} with ${repetitions} repetitions"
                extra_json="${raw_dir}/${target}.${round}.${repetitions}.json"
                extra_log="${raw_dir}/${target}.${round}.${repetitions}.log"
                run_benchmark "${target}" "${repetitions}" "${pattern}" "${extra_json}" "${extra_log}"
                python3 "${SCRIPT_DIR}/merge_benchmark_results.py" \
                    --target "${raw_json}" --update "${extra_json}" --quiet
                python3 "${SCRIPT_DIR}/merge_benchmark_results.py" \
                    --target "${raw_log}" --update "${extra_log}"
            done
        done

        noisy=$(select_repetitions "${raw_json}" --report)
        if [[ -n ${noisy} ]]; then
            warn "these benchmarks stay above ${TARGET_PRECISION}% even at ${MAX_REPETITIONS} repetitions:"
            sed 's/^/    /' <<<"${noisy}"
        fi
    fi

    stamp_max_frequency "${raw_json}"

    if [[ -n ${FILTER} ]]; then
        info "Merging ${target} into ${RESULT_DIR}"
        python3 "${SCRIPT_DIR}/merge_benchmark_results.py" \
            --target "${RESULT_DIR}/${target}.json" --update "${raw_json}"
        python3 "${SCRIPT_DIR}/merge_benchmark_results.py" \
            --target "${RESULT_DIR}/${target}.log" --update "${raw_log}"
    else
        info "Replacing ${RESULT_DIR}/${target}.{json,log}"
        python3 "${SCRIPT_DIR}/merge_benchmark_results.py" \
            --target "${RESULT_DIR}/${target}.json" --update "${raw_json}" --compare-only
        cp -- "${raw_json}" "${RESULT_DIR}/${target}.json"
        cp -- "${raw_log}" "${RESULT_DIR}/${target}.log"
    fi
done

#
# CHECKS
#
for pair in "${CPP_READ} ${C_READ}" "${CPP_WRITE} ${C_WRITE}"; do
    read -r cpp_target c_target <<<"${pair}"
    [[ -f ${RESULT_DIR}/${cpp_target}.json && -f ${RESULT_DIR}/${c_target}.json ]] || continue
    info "Comparing ${cpp_target} against ${c_target}"
    if ! python3 "${SCRIPT_DIR}/check_results.py" \
        --json "${RESULT_DIR}/${cpp_target}.json" "${RESULT_DIR}/${c_target}.json"; then
        warn "cpp-yyjson wraps the same backend, so it should not lose to the raw C API; re-measure those rows before committing"
    fi
done

#
# CHARTS
#
if [[ ${DO_PLOT} -eq 1 ]]; then
    missing=0
    for target in "${CPP_READ}" "${CPP_WRITE}" "${C_READ}" "${C_WRITE}"; do
        [[ -f ${RESULT_DIR}/${target}.json ]] || missing=1
    done
    if [[ ${missing} -eq 1 ]]; then
        warn "not all result files are present; skipping the charts"
    else
        info "Regenerating ${IMAGE_DIR}"
        python3 "${SCRIPT_DIR}/plot_benchmarks.py" \
            --read-json "${RESULT_DIR}/${CPP_READ}.json" "${RESULT_DIR}/${C_READ}.json" \
            --write-json "${RESULT_DIR}/${CPP_WRITE}.json" "${RESULT_DIR}/${C_WRITE}.json" \
            --output-dir "${IMAGE_DIR}"
    fi
fi

info "Done"
printf '  changed files:\n'
git status --porcelain -- "${RESULT_DIR}" "${IMAGE_DIR}" 2>/dev/null | sed 's/^/    /' || true
printf '\n  Review the diff and update the environment paragraph of %s if the\n' "${DOC}"
printf '  machine, the compiler or the vcpkg baseline changed.\n'
