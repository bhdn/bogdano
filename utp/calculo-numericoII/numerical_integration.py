#!/usr/bin/env python
"""
Stupid way to implement numerical integraton. Don't do it. RTFM of pylab
and do things in the right way.

Mostly based on:
http://people.hofstra.edu/faculty/Stefan_waner/RealWorld/integral/numint.html
"""

__author__ = "Bogdano Arendartchuk <debogdano@gmail.com>"

from pylab import *

# Trapezoid error:
# Error **no larger than**:
#
#        (b - a)^3
# Err <= ---------|f''(M)|
#         12n^2
#
# (12n^2)*Err = (b -a ^3)|f''(M)|
#
#             +--------------------
# n =  -+    /((b - a)^3)*|f''(M)|
#        \  / --------------------
#         \/   12*Err


def _max(f, a, b, err):
    x = arange(a, b, err) #FIXME too stupid!
    return f(x).max() #XXX abs?

def intr_trapezoid(a, b, f, m):
    s = 0.0
    it = 0
    h = (b - a) / m 
    # n esta implicito!
    xi = a
    while it <= m:
        if it == 0 or it == m:
            s += f(xi)
        else:
            s += 2.0 * f(xi)
        it += 1 #FIXME just calculate for 0 and N
        xi += h
    return (h / 2.0) * s

def intr_simpson13(a, b, f, m):
    s = 0.0
    it = 0
    h = (b - a) / m 
    xi = a
    while it <= m:
        if it == 0 or it == m:
            s += f(xi)
        elif it % 2 != 0:
            s += 4.0 * f(xi)
        else:
            s += 2.0 * f(xi)
        it += 1
        xi += h
    return ( (b-a) / (3.0*(m+1)) ) * s

def iternum_trapezoid(a, b, fII, maxerr):
    fM = _max(fII, a, b, maxerr)
    return sqrt((((b-a)**3.0)*fM)/(12*maxerr))

def err_trapezoid(a, b, fII, iternum):
    fM = _max(fII, a, b, 1e-5) #FIXME don't use constant err
    return (((b-a)**5.0)*fM)/(12.0*iternum**2.0)


def iternum_simpson13(a, b, fIV, maxerr):
    fM = _max(fIV, a, b, maxerr)
    return sqrt( sqrt( (((b-a)**5.0)*fM)/(180*maxerr) ))

def err_simpson13(a, b, fIV, iternum):
    fM = _max(fIV, a, b, 1e-5) #FIXME
    return (((b-a)**5.0)*fM)/(180*iternum**4.0)


def iternum_simpson38(a, b, fIV, maxerr):
    fM = _max(fIV, a, b, maxerr)
    return sqrt( sqrt( (((b-a)**5.0)*fM)/(80*maxerr) ))

def err_simpson38(a, b, fIV, iternum):
    fM = _max(fIV, a, b, 1e-5) #FIXME
    return (((b-a)**5.0)*fM)/(80*iternum**4.0)


if __name__ == "__main__":
    def f(x):
        return exp(x) + sin(x)
    def fII(x):
        return exp(x) - sin(x)
    def fIV(x):
        return exp(x) - sin(x)
    maxerr = 1e-2
    a = 0.0
    b = 2.0
    n_simpson13 = iternum_simpson13(a, b, fIV, maxerr)
    n_simpson38 = iternum_simpson38(a, b, fIV, maxerr)
    n_trapezoid = iternum_trapezoid(a, b, fII, maxerr)
    print "iternum_simpson13:", n_simpson13
    print "iternum_simpson38:", n_simpson38
    print "iternum_trapezoid:", n_trapezoid
    
    print "intr_trapezoid:", intr_trapezoid(a, b, f, n_trapezoid)
    print "intr_simpson13:", intr_simpson13(a, b, f, n_simpson13)

    print "err_trapezoid:", err_trapezoid(a, b, fII, n_trapezoid)
    print "err_simpson13:", err_simpson13(a, b, fIV, n_simpson13)
    print "err_simpson38:", err_simpson38(a, b, fIV, n_simpson38)

