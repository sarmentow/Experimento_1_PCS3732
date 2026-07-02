#!/usr/bin/env bash
set -u

tmp=$(mktemp)
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
trap 'rm -f "$tmp"' EXIT

if [[ ! -x "$script_dir/a.out" ]]; then
  echo "Error: a.out not found or not executable at: $script_dir/a.out" >&2
  exit 1
fi

failures=0
for i in $(seq 1 100); do
  if ! (time -p "$script_dir/a.out" > /dev/null) 2>> "$tmp"; then
    ((failures += 1))
  fi
done

awk '
  /^real[[:space:]]+/ {
    n++
    sum += $2
    sumsq += $2 * $2
  }
  END {
    if (n == 0) {
      print "No valid timing samples collected."
      exit 1
    }
    mean = sum / n
    variance = (n > 1) ? (sumsq - sum * sum / n) / (n - 1) : 0
    std = (variance >= 0) ? sqrt(variance) : 0
    printf "Runs: %d\n", n
    printf "Avg: %.6f s\n", mean
    printf "Std dev: %.6f s\n", std
  }
' "$tmp"

if [ "$failures" -gt 0 ]; then
  echo "Failed runs: $failures (ignored in timing stats)"
fi
