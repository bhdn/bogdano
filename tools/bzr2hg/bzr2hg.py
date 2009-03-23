#!/usr/bin/env python
import sys
import os
import stat
import optparse
import subprocess
import locale
import logging
from cStringIO import StringIO

from bzrlib import branch, diff, log, revisionspec

__author__ = "Bogdano Arendartchuk <debogdano@gmail.com>"
__license__ = "GPL"

encode_locale = locale.getpreferredencoding()

logger = logging.getLogger("bzr2hg")

class Error(Exception):
    pass

def cmd(args, noerror=False, write=None, workdir=None):
    if workdir:
        lastdir = os.getcwd()
        os.chdir(workdir)
    try:
        logger.debug("running command: %s", args)
        pipe = subprocess.Popen(args, stdout=subprocess.PIPE,
                stdin=subprocess.PIPE, stderr=subprocess.STDOUT, shell=False)
        if write:
            out, _ = pipe.communicate(write)
        else:
            out = pipe.stdout.read()
        logger.debug("output: %s", out)
        error = pipe.wait()
        if error != 0 and not noerror:
            raise Error, "the command %s failed with %d: %s" % (args, error,
                    out)
    finally:
        if workdir:
            os.chdir(lastdir)
    return out

def apply_patch(dir, changes):
    logger.debug("patching %s with: %s", dir, changes)
    cmd(["patch", "-d", dir, "-p1"], write=changes)

def hg(*args):
    return cmd(["hg"] + list(args))

def bzr_get_changeset(branch, revid):
    # the changes
    delta = branch.repository.get_revision_delta(revid)
    out = StringIO()
    revspec = "before:revid:" + str(revid)
    before = revisionspec.RevisionSpec.from_string(revspec)
    before_revid = before.as_revision_id(branch)
    tree1 = branch.repository.revision_tree(before_revid)
    tree2 = branch.repository.revision_tree(revid)
    diff.show_diff_trees(tree1, tree2, out)
    changes = out.getvalue()
    if not "+++" in changes:
        # otherwise patch will complain about no useful text in the
        # generated diff
        changes = None
    # the log message
    revision = branch.repository.get_revision(revid)
    log = revision.message.encode("utf8", "replace") # hg enforces utf-8
    author = revision.get_apparent_author()
    date = "%d %d" % (revision.timestamp, -revision.timezone) # bizarre hg format
    # collect the file attributes changes
    inventory = dict(tree2.inventory.entries())
    attr_changed = [path
                    for (path, fileid, kind, cont, attr)
                    in delta.modified if attr]
    attr_changed.extend(path
                    for (path, path2, fileid, kind, cont, attr)
                    in delta.renamed if attr)
    attr_changed.extend(path
                    for path, fileid, kind in delta.added
                    if inventory[path].executable)
    executables = [(path, inventory[path].executable) for path in attr_changed]
    added = []
    newdirs = []
    for name, fileid, type in delta.added:
        encoded = name.encode(encode_locale)
        added.append(encoded)
        if type == "directory":
            newdirs.append(encoded)
    removed = [name.encode(encode_locale)
            for name, fileid, type in delta.removed]
    renamed = [(old.encode(encode_locale), new.encode(encode_locale))
            for old, new, fileid, type, _, __ in delta.renamed]
    return (date, author, log, changes, added, removed, renamed, executables,
            newdirs)

def hg_add(hg_dir, files):
    args = ["hg", "add"]
    files = [os.path.join(hg_dir, file) for file in files]
    args.extend(files)
    cmd(args, workdir=hg_dir)

def hg_rm(hg_dir, files):
    args = ["hg", "rm"]
    files = [os.path.join(hg_dir, file) for file in files]
    args.extend(files)
    cmd(args, workdir=hg_dir)

def hg_commit(hg_dir, log, author=None, date=None):
    # seems -F - is broken on subversion for now
    args = ["hg", "ci", "-l", "-"]
    if author:
        args.append("--user")
        args.append(author)
    if date:
        args.append("--date")
        args.append(date)
    args.append(".")
    cmd(args, write=log, workdir=hg_dir)

def hg_mv(hg_dir, old, new):
    oldpath = os.path.join(hg_dir, old)
    newpath = os.path.join(hg_dir, new)
    cmd(["hg", "mv", oldpath, newpath], workdir=hg_dir)

def hg_chmod(hg_dir, executables):
    for path, executable in executables:
        path = os.path.join(hg_dir, path)
        mode = os.stat(path).st_mode
        if executable:
            mode |= 0100
        else:
            mode &= ~0111
        logger.debug("chmod %s to %o" % (path, mode))
        os.chmod(path, mode)

def hg_mkdir(hg_dir, dirs):
    for dir in dirs:
        path = os.path.join(hg_dir, dir)
        if not os.path.exists(path):
            logger.debug("creating directory %s" % path)
            os.mkdir(path)

def hg_tag(hg_dir, tag, date=None):
    args = ["hg", "tag"]
    if date:
        args.append("--date")
        args.append(date)
    args.append(tag)
    cmd(args, workdir=hg_dir)

def check_missing(hg_dir, files):
    for file in files:
        path = os.path.join(hg_dir, file)
        if not os.path.exists(path):
            # the file was not created by the patch, possibly because it
            # should be empty
            logger.debug("creating empty file: %s" % path)
            open(path, "w").close()

def hg_push_changeset(hg_dir, 
        (date, author, log, changes, added, removed, renamed, executables,
            newdirs),
        commit=True):
    if renamed:
        for old, new in renamed:
            hg_mv(hg_dir, old, new)
    if newdirs:
        hg_mkdir(hg_dir, newdirs)
    if changes:
        apply_patch(hg_dir, changes)
    if added:
        check_missing(hg_dir, added)
        hg_add(hg_dir, added)
    if removed and commit:
        hg_rm(hg_dir, removed)
    if executables:
        hg_chmod(hg_dir, executables)
    if commit:
        hg_commit(hg_dir, log, author=author, date=date)

def hg_ensure_untouched(hg_dir):
    # hg st would be better
    #FIXME we don't know how to do it for hg
    pass

def bzr_get_subrevs(source_br, rev):
    subrevs = log.calculate_view_revisions(source_br, rev, rev, 'forward',
            None, True, True)
    revids = [revid for revid, subrev, depth in subrevs
                    if depth == 1] # we don't want the merge itself
    return revids

def convert(source_bzr, dest_hg, subcommit=[], start_rev=None,
        end_rev=None, commit=False):
    """Converts commits from a bzr branch to a hg working copy"""
    prev = 0
    hg_ensure_untouched(dest_hg)
    source_br = branch.Branch.open(source_bzr)
    if end_rev is None:
        end_rev = source_br.revno()
        logger.debug("latest bzr revision: %s", end_rev)
    source_tags = source_br.tags.get_reverse_tag_dict()
    for rev in xrange(start_rev, end_rev+1):
        if rev in subcommit:
            revs = bzr_get_subrevs(source_br, rev)
        else:
            revs = [source_br.get_rev_id(rev)]
        for subrev in revs:
            (date, author, log, changes, added, removed, renamed, executables,
                    newdirs) = bzr_get_changeset(source_br, subrev)
            hg_push_changeset(dest_hg,
                    (date, author, log, changes, added, removed, renamed,
                        executables, newdirs), commit)
            tags = source_tags.get(subrev)
            if tags:
                for tag in tags:
                    tag = tag.encode(encode_locale)
                    hg_tag(dest_hg, tag, date=date)
            logger.info("pushed revision %s:%s" % (rev, subrev))

def increase_verbosity(*a, **kw):
    logger.setLevel(logging.DEBUG)

def parse_options(args):
    banner = "Commits a set of bzr changesets onto a hg working copy"
    usage = "%prog -r REV -s BZRBRANCH -d HGREPO"
    parser = optparse.OptionParser(description=banner, usage=usage)
    parser.add_option("-s", "--source", type="string",
            help="source bzr branch")
    parser.add_option("-d", "--dest", type="string",
            help="destination hg working copy")
    parser.add_option("-r", "--start-rev", type="int", default=1,
            help="start working from the given revision (default: 1)")
    parser.add_option("-e", "--end-rev", type="int", default=None,
            help="stops working on the given revision")
    parser.add_option("-i", "--interesting", type="int", default=[],
            action="append", dest="subcommit",
            help="commit all the merged revisions from the given revision")
    parser.add_option("-n", "--dry-run", default=True,
            action="store_false", dest="commit",
            help="do not commit changes (but leaves the working copy "\
                 "modified)")
    parser.add_option("-v", "--verbose", action="callback",
            callback=increase_verbosity)
    opts, args = parser.parse_args()
    if not (opts.source and opts.dest):
        parser.error("both options source and dest are required")
    return opts, args

def main(args):
    try:
        logging.basicConfig(level=logging.INFO)
        opts, args = parse_options(args)
        convert(opts.source, opts.dest, opts.subcommit, opts.start_rev,
                opts.end_rev, opts.commit)
    except Error, e:
        sys.stderr.write("error: %s\n" % e)
        return 1
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

# vim:ts=4:sw=4:et
