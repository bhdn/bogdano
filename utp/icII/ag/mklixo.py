#!/usr/bin/env python
import os
import sys
import random


def create_stuff(nfiles):
    for nfile in xrange(nfiles):
        name = "file-%05d.METAL" % nfile
        f = open(name, "w")
        for block in xrange(random.randint(1, 500)):
            content = "M" * (1024)
            f.write(content)
        f.close()

if __name__ == "__main__":
    # <nfiles> <range>
    create_stuff(int(sys.argv[1]))
