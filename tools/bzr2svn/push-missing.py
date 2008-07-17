#!/usr/bin/env python
import sys
import os
import optparse
import subprocess
import locale
from cStringIO import StringIO
from bzrlib import branch, diff, log, revisionspec

encode_locale = locale.getpreferredencoding()

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

def apply_patch(dir, changes):
    cmd(["patch", "-d", dir, "-p1"], write=changes)

def svn(*args):
    return cmd(["svn"] + list(args))

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

def bzr_get_changeset(branch, rev):
    # the changes
    out = StringIO()
    rev_id = branch.get_rev_id(rev)
    before = revisionspec.RevisionSpec.from_string("before:" + str(rev))
    before_revid = before.as_revision_id(branch)
    tree1 = branch.repository.revision_tree(before_revid)
    tree2 = branch.repository.revision_tree(rev_id)
    diff.show_diff_trees(tree1, tree2, out)
    changes = out.getvalue()
    # the log message
    log = branch.repository.get_revision(rev_id).message
    delta = branch.repository.get_revision_delta(rev_id)
    added = [name.encode(decode_locale)
            for name, fileid, type, _, __ in delta.added]
    removed = [name.encode(decode_locale)
            for name, fileid, type, _, __ in delta.removed]
    return log, changes, added, removed

def svn_add(svn_dir, files):
    args = ["svn", "add", "--force"]
    files = [os.path.join(svn_dir, file) for file in files]
    args.extend(files)
    cmd(args)

def svn_rm(svn_dir, files):
    args = ["svn", "rm"]
    files = [os.path.join(svn_dir, file) for file in files]
    args.extend(files)
    cmd(args)

def svn_commit(svn_dir, log):
    # seems -F - is broken on subversion for now
    cmd(["svn", "ci", svn_dir, "-F", "/dev/stdin"], write=log)

def svn_push_changeset(svn_dir, (log, changes, added, removed)):
    apply_patch(svn_dir, changes)
    svn_add(svn_dir, added)
    svn_rm(svn_dir, removed)
    #svn_commit(svn_dir, log)

def svn_ensure_untouched(svn_dir):
    # svn st would be better
    out = cmd(["svn", "diff", svn_dir])
    if out:
        raise Error, "sorry mate, I refuse to work on a working copy "\
                "with uncommited changes"

def bzr_get_subrevs(source_br, rev):
    subrevs = log.calculate_view_revisions(source_br, rev, rev, 'reverse',
            None, True, True)
    return [rev for revid, rev, depth in subrevs[::-1]]

def convert(source_bzr, dest_svn, subcommit=[], start_rev=None,
        end_rev=None):
    """Converts commits from a bzr branch to a svn working copy"""
    prev = 0
    svn_ensure_untouched(dest_svn)
    source_br = branch.Branch.open(source_bzr)
    if end_rev is None:
        end_rev = source_br.revno()
    for rev in xrange(start_rev, end_rev+1):
        if rev in subcommit:
            revs = bzr_get_subrevs(source_br, rev)
        else:
            revs = [rev]
        for subrev in revs:
            log, changes, added, removed = \
                    bzr_get_changeset(source_br, subrev)
            svn_push_changeset(dest_svn, (log, changes, added, removed))
            source_br.tags.set_tag("pushed-svn",
                    source_br.get_rev_id(subrev))
            bzr_tag(source_bzr, "pushed-svn", subrev)
            print "pushed revision %s" % (subrev)

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
