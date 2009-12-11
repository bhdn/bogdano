#!/usr/bin/env python
import os
import sys
import random


def create_stuff(nfiles):
    for nfile in xrange(nfiles):
        name = "file-%05d.METAL" % nfile
        f = open(name, "w")
        size = random.randint(1, 2000) * 1024
        f.seek(size-1)
        f.write("\0")
        f.close()

if __name__ == "__main__":
    # <nfiles> <range>
    create_stuff(int(sys.argv[1]))
