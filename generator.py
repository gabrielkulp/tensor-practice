#!/usr/bin/env python
import random
import sys
from operator import mul
from functools import reduce

if len(sys.argv) < 3:
    print("Sparse tensor generator\n")
    print("USAGE:")
    print("   ", sys.argv[0], "DENSITY MODE1 [MODE2]...\n")
    print("EXAMPLE:")
    print("    To generate a 10x10 tensor with around 5 nonzero entries, use")
    print("   ", sys.argv[0], "0.05 10 10")
    exit()

shape = list(map(int, sys.argv[2:]))
density = float(sys.argv[1])
volume = reduce(mul, shape, 1)
nnz = int(volume * density)
T = {}

for _ in range(nnz):
    coords = tuple([random.randint(0, m-1) for m in shape])
    T[coords] = float(random.randint(1, 50))

print("order: ", len(shape))
print("shape: ", ", ".join(map(str, shape)))
print("values:")
for (key, val) in T.items():
    print(", ".join(map(str, key)) + ",", val)
