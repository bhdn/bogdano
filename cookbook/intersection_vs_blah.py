#!/usr/bin/env python
"""
Compares two oneliners that check if one item of a given set is member of
another. Then another test with a plain "for" loop (as it does not visit
all items to check it).

For small sets being checked ("items"), creating set just to check if it is
inside the larger set is faster than using one generator.

Bogdano Arendartchuk <debogdano@gmail.com>
"""
import timeit
import random

ri = random.randint

data = frozenset(str(i) for i in xrange(100000,999999))
items = frozenset(("325236", "343453", "497219", 
            "not-found", "239160", "another-miss", "999998"))

def test_intersection():
    return bool(data.intersection(items))


def test_sum():
    return bool(sum(1 for item in items if item in data))

def test_for():
    for item in items:
        if item in data:
            return True
    return False

timer_inter = timeit.Timer("test_intersection()",
                           "from __main__ import test_intersection")
timer_sum = timeit.Timer("test_sum()",
                         "from __main__ import test_sum")
timer_for = timeit.Timer("test_for()",
                         "from __main__ import test_for")

def fmt(n):
    return n
import pprint
res_iter = map(fmt, timer_inter.repeat(10, 1))
res_sum = map(fmt, timer_sum.repeat(10, 1))
res_for = map(fmt, timer_for.repeat(10, 1))
pprint.pprint(zip(res_iter, res_sum, res_for))
