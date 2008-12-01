#!/usr/bin/python
# 
#
import os
import glob
import sys
import subprocess

SUCCESSDIR = "tests/codegen-mepa/success"
FAILDIR = "tests/codegen-mepa/fail"

if os.name == "win32":
    TESTER = "toscal.exe"
else:
    TESTER = "./toscal"

def check(test, should_succeed):
    outputpath = test + "-output"
    oldoutput = open(outputpath).read()
    testinput = open(test)
    proc = subprocess.Popen([TESTER, "-W"], stdin=testinput,
            stderr=subprocess.PIPE,
            stdout=subprocess.PIPE)
    err = proc.wait()
    output = proc.stderr.read() + proc.stdout.read() # crap!
    testinput.close()
    failed = False
    if (should_succeed and err != 0) or (not should_succeed and err == 0):
        print "FAILED",
        failed = True
    if output != oldoutput:
        print "DIFFER",
        failed = True
    if not failed:
        print "GOOD",
    print test
    return not failed

def run(testsdir, should_succeed=True):
    tests = os.path.join(testsdir, "*.pas")
    errors = 0
    for path in glob.glob(tests):
        if not check(path, should_succeed):
            errors += 1
    return errors

def main():
    errors = run(SUCCESSDIR, True)
    errors += run(FAILDIR, False)
    print "errors:", errors
    if errors != 0:
        sys.exit(1)

if __name__ == "__main__":
    main()
