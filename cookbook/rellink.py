"""
Finds the common parent directory in order to have the closest relative
symlinks. Useful when the links are going to be created by non-humans.

Not using os.path.commonprefix just because it is character-based and I'm
not in which cases it will be unapropriated.

Bogdano Arendartchuk <debogdano@gmail.com>
may/2007
"""

import os

def rellink(src, dst):
    """Creates relative symlinks

    It will find the common ancestor and append to the src path.
    """
    asrc = os.path.abspath(src)
    adst = os.path.abspath(dst)
    csrc = asrc.split(os.path.sep)
    cdst = adst.split(os.path.sep)
    dstname = cdst.pop()
    i = 0
    l = min(len(csrc), len(cdst))
    while i < l:
        if csrc[i] != cdst[i]:
            break
        i += 1
    dstextra = len(cdst[i:])
    steps = [os.path.pardir] * dstextra
    steps.extend(csrc[i:])
    return os.path.sep.join(steps)
