#!/usr/bin/env python
import sys

g = -1
fit = 0.0
for line in sys.stdin:
    if line.startswith("fitness media: "):
        f = line.split()
        raw = f[2]
        fit = float(raw)
        print "%d\t %.2f" % (g, fit)
    if line.startswith("geracao: "):
        f = line.split()
        raw = f[1]
        g = int(raw)
