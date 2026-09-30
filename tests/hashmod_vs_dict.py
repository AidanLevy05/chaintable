import os
import sys
import time

# Find hashmod.*.so whether this file is in the project root or in tests/
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
sys.path.insert(0, os.path.join(here, ".."))

import hashmod

SIZES = [1_000, 10_000, 100_000, 1_000_000]   # number of keys per run
REPEATS = 5                        # take the best of N runs


def make_keys(n):
    return [f"key{i}" for i in range(n)]


def new_hashmod():
    return hashmod.HashTable(8)


def new_dict():
    return {}


def prefill(table, keys):
    for i, k in enumerate(keys):
        table[k] = i


# ---- benchmarks: each takes (table, keys) ----------------------------
# Same code runs on both tables, since dict syntax works on both.

def bench_insert(table, keys):
    for i, k in enumerate(keys):
        table[k] = i


def bench_get(table, keys):
    for k in keys:
        table[k]


def bench_delete(table, keys):
    for k in keys:
        del table[k]


# Each entry: (name, function, needs_prefilled_table)
BENCHMARKS = [
    ("insert", bench_insert, False),
    ("get",    bench_get,    True),
    ("delete", bench_delete, True),
]


def time_op(make_table, fn, keys, prefilled):
    """Best-of-REPEATS time for fn(table, keys), excluding setup.

    A fresh table is built for every repeat (delete destroys it), and the
    clock only runs around fn itself.
    """
    best = float("inf")
    for _ in range(REPEATS):
        table = make_table()
        if prefilled:
            prefill(table, keys)
        start = time.perf_counter()
        fn(table, keys)
        best = min(best, time.perf_counter() - start)
    return best


def run(fn, prefilled, n):
    keys = make_keys(n)
    return (time_op(new_hashmod, fn, keys, prefilled),
            time_op(new_dict, fn, keys, prefilled))


def main():
    print(f"{'op':<8}{'n':>10}{'hashmod (s)':>14}{'dict (s)':>12}{'ratio':>8}")
    for n in SIZES:
        for name, fn, prefilled in BENCHMARKS:
            t_mine, t_dict = run(fn, prefilled, n)
            ratio = t_mine / t_dict if t_dict else float("nan")
            print(f"{name:<8}{n:>10}{t_mine:>14.6f}{t_dict:>12.6f}{ratio:>7.1f}x")


if __name__ == "__main__":
    main()