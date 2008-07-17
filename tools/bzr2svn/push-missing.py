#!/usr/bin/env python
import sys
import os
import optparse
import subprocess

class Error(Exception):
    pass

def cmd(*args):
    pipe = subprocess.Popen(args, stdout=subprocess.PIPE, shell=False)
    error = pipe.wait()
    if error != 0:
        output = pipe.stdout.read()
        raise Error, "the command %s failed with %d: %s" % (args, error,
                output)
    return error, pipe.stdout

def svn(*args):
    return cmd("svn", *args)

def bzr(*args):
    return cmd("bzr", *args)

def parse_options(args):
    banner = "Commits a set of bzr changesets to a svn working copy"
    parser = optparse.OptionParser(description=banner)
    parser.add_option("-s", "--source", type="string",
            help="source bzr branch")
    parser.add_option("-d", "--dest", type="string",
            help="destination svn working copy")
    opts, args = parser.parse_args()
    if not (opts.source and opts.dest):
        parser.error("both options source and dest are required")
    return opts, args

def main(args):
    try:
        opts, args = parse_options(args)
    except Error, e:
        sys.stderr.write("error: %s\n" % e)
        return 1
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
