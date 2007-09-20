#!/usr/bin/python

import time
import random
import pyosd

def show(op, val1, val2, resp):
    osd = pyosd.osd("-*-helvetica-*-r-*-*-24-*-*-*-*-*-*-*", "green")
    osd.set_pos(pyosd.POS_MID)
    osd.set_align(pyosd.ALIGN_CENTER)
    question = "%d %s %d ???" % (val1, op, val2)
    osd.display(question)
    delay = 5
    if val1 > 15 or val2 > 15:
        if op in "*/":
            delay = 17
        else:
            delay = 9
    time.sleep(delay)
    osd.set_colour("red")
    osd.display("%s => %s!!!" % (question, resp))
    time.sleep(5)

def conta():
    ops = {"/": int.__div__, 
           "+": int.__add__,
           "-": int.__sub__,
           "*": int.__mul__ }
    while True:
        op = random.choice(ops.keys())
        val1 = random.randint(0, 100)
        val2 = random.randint(0, 100)
        if op != "/" and val2 != 0:
            break
    resp = ops[op](val1, val2)
    return op, val1, val2, resp

def main():
    show(*conta())

if __name__ == "__main__":
    main()
