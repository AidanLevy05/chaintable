import sys

n = 10_000_000
if sys.argv[1] == "hashmod":
    import hashmod
    t = hashmod.HashTable(8)
else:
    t = {}

for i in range(n):
    t[f"key{i}"] = i
