#!/usr/bin/env python
import sys
import os
import optparse
import subprocess

#TODO list of special commits whose subcommits must be commited
#     individually

class Error(Exception):
    pass

def cmd(args, noerror=False, write=None):
    print "running: %r" % args
    pipe = subprocess.Popen(args, stdout=subprocess.PIPE,
            stdin=subprocess.PIPE, stderr=subprocess.STDOUT, shell=False)
    if write:
        out, _ = pipe.communicate(write)
    else:
        out = pipe.stdout.read()
    error = pipe.wait()
    if error != 0 and not noerror:
        raise Error, "the command %s failed with %d: %s" % (args, error,
                out)
    return out

def diffstat_l(changes):
    out = cmd(["diffstat", "-l", "-p0"], write=changes)
    lines = out.splitlines()
    return lines

def apply_patch(dir, changes):
    cmd(["patch", "-d", dir, "-p0"], write=changes)

def svn(*args):
    return cmd(["svn"] + list(args))

def bzr(subcmd, *args):
    noerror = subcmd == "diff"
    args = ["bzr", subcmd] + list(args)
    return cmd(args, noerror=noerror)

def parse_options(args):
    banner = "Commits a set of bzr changesets to a svn working copy"
    parser = optparse.OptionParser(description=banner)
    parser.add_option("-s", "--source", type="string",
            help="source bzr branch")
    parser.add_option("-d", "--dest", type="string",
            help="destination svn working copy")
    parser.add_option("-r", "--start-rev", type="int", default=None,
            help="start working from the given revision")
    parser.add_option("-e", "--end-rev", type="int", default=None,
            help="stops working on the given revision")
    opts, args = parser.parse_args()
    if not (opts.source and opts.dest):
        parser.error("both options source and dest are required")
    return opts, args

def bzr_get_changeset(bzr_dir, rev):
    changes = bzr("diff", "-c", str(rev), bzr_dir)
    log = bzr("log", "-r", str(rev))
    return log, changes

def bzr_tag(bzr_dir, name, rev):
    cmd(["bzr", "tag", "--force", "-r", str(rev), "-d", bzr_dir])

def bzr_latest_rev(bzr_dir):
    out = cmd(["bzr", "log", "-r", "-1", "--line"])
    rawrev = out.split(":", 1)[0]
    rev = int(rawrev)
    return rev

def svn_add(svn_dir, files):
    args = ["svn", "add", "--force"]
    files = [os.path.join(svn_dir, file) for file in files]
    args.extend(files)
    cmd(args)

def svn_commit(svn_dir, log):
    # seems -F - is broken on subversion for now
    cmd(["svn", "ci", svn_dir, "-F", "/dev/stdin"], write=log)

def svn_push_changeset(svn_dir, (log, changes)):
    files = diffstat_l(changes)
    apply_patch(svn_dir, changes)
    svn_add(svn_dir, files)
    svn_commit(svn_dir, log)

def svn_ensure_untouched(svn_dir):
    # svn st would be better
    out = cmd(["svn", "diff", svn_dir])
    if out:
        raise Error, "sorry mate, I refuse to work on a working copy "\
                "with uncommited changes"

def convert(source_bzr, dest_svn, subcommit=[], start_rev=None,
        end_rev=None):
    """Converts commits from a bzr branch to a svn working copy"""
    prev = 0
    svn_ensure_untouched(dest_svn)
    if end_rev is None:
        end_rev = bzr_latest_rev(source_bzr)
    for rev in xrange(start_rev, end_rev+1):
        if rev in subcommit:
            #FIXME obtain here the list of subrevisions
            revs = [rev]
        else:
            revs = [rev]
        for subrev in revs:
            log, changes = bzr_get_changeset(source_bzr, subrev)
            svn_push_changeset(dest_svn, (log, changes))
            bzr_tag(source_bzr, "pushed-svn", subrev)
            print "pushed revision %s: %s" % (subrev,
                    changes.split("\n")[0])

def main(args):
    try:
        opts, args = parse_options(args)
        convert(opts.source, opts.dest, [], opts.start_rev, opts.end_rev)
    except Error, e:
        sys.stderr.write("error: %s\n" % e)
        return 1
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
