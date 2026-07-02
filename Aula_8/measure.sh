#!/usr/bin/env bash

tmp=$(mktemp)
for i in $(seq 1 100); do
  (/usr/bin/time -f '%e' ./a.out > /dev/null) 2>> "$tmp"
done

awk '{
  n++
  sum += $1
  sumsq += $1 * $1
}
END {
  mean = sum / n
  std = sqrt((sumsq - sum * sum / n) / (n - 1))
  printf "Runs: %d\nAvg: %.6f s\nStd dev: %.6f s\n", n, mean, std
}' "$tmp"

rm -f "$tmp"
