#!/usr/bin/env bash

set -euo pipefail

tmp=$(mktemp)
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
trap 'rm -f "$tmp"' EXIT

if [[ ! -x "$script_dir/a.out" ]]; then
  echo "Error: a.out not found or not executable at: $script_dir/a.out" >&2
  echo "Run this script from a directory that contains a.out or place both files together." >&2
  exit 1
fi

for i in $(seq 1 100); do
  (/usr/bin/time -f '%e' "$script_dir/a.out" > /dev/null) 2>> "$tmp"
done

python3 - "$tmp" <<'PY'
import math
import sys
import re

time_re = re.compile(r"^\d+(\.\d+)?$")

with open(sys.argv[1], "r", encoding="utf-8") as f:
    times = [
        float(value.strip())
        for value in f
        if value.strip() and time_re.match(value.strip())
    ]

n = len(times)
if n == 0:
    print("No valid timing samples collected.")
    print("Check if a.out failed to run; see errors above in the temporary file path.")
    sys.exit(1)

mean = sum(times) / n
std = math.sqrt(sum((t - mean) ** 2 for t in times) / (n - 1)) if n > 1 else 0.0

print(f"Runs: {n}")
print(f"Avg: {mean:.6f} s")
print(f"Std dev: {std:.6f} s")
PY
