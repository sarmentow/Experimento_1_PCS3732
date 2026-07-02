#!/usr/bin/env bash

tmp=$(mktemp)
trap 'rm -f "$tmp"' EXIT

for i in $(seq 1 100); do
  (/usr/bin/time -f '%e' ./a.out > /dev/null) 2>> "$tmp"
done

python3 - "$tmp" <<'PY'
import math
import sys

with open(sys.argv[1], "r", encoding="utf-8") as f:
    times = [float(line.strip()) for line in f if line.strip()]

n = len(times)
if n == 0:
    print("No timing samples collected.")
    sys.exit(1)

mean = sum(times) / n
std = math.sqrt(sum((t - mean) ** 2 for t in times) / (n - 1)) if n > 1 else 0.0

print(f"Runs: {n}")
print(f"Avg: {mean:.6f} s")
print(f"Std dev: {std:.6f} s")
PY
