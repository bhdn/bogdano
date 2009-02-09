#!/usr/bin/python
# flatten a given directory list: generates a flat file list, but with each
# file name prefixed with a counter, in order to not lose the in-directory
# file order
#
# Bogdano Arendarchuk <bhdn@ukr.net>
#
import sys
import os
import optparse
import fnmatch

def flatten(srcs, dest, filter=None):
    count = 0
    for src in srcs:
        for root, dirs, files in os.walk(src):
            if filter is not None:
                files = fnmatch.filter(files, filter)
            for name in files:
                path = os.path.join(root, name)
                newname = "%04d-%s" % (count, name)
                newpath = os.path.join(dest, newname)
                yield path, newpath
                count += 1

def parse_options(args):
    parser = optparse.OptionParser(usage="%prog [<src>, ..., <srcN> <dest>]")
    parser.add_option("-n", "--dry-run", dest="dry_run", default=False,
            action="store_true", help="Don't harm anything")
    parser.add_option("-l", "--link", dest="link", default=False,
            action="store_true", help="Use symlinks")
    parser.add_option("-f", "--filter", dest="filter", type="string",
            default=None, help="Filter file names matching a glob")
    opts, args = parser.parse_args(args)
    if len(args) < 2:
        parser.error("invalid number of args")
    return opts, args

def main(args):
    opts, args = parse_options(sys.argv[1:])
    dest = args[-1]
    srcs = args[:-1]
    if not os.path.exists(dest):
        sys.stderr.write("not found: %s\n" % dest)
        return 1
    for old, new in flatten(args[0], dest, opts.filter):
        if not opts.dry_run:
            if opts.link:
                os.symlink(old, new)
            else:
                os.link(old, new)
        print old, "=>", new
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv))
