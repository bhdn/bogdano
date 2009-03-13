#!/bin/awk -f
# 
# This script checks whether the generated kernel ABI for the built kernel
# has changed since the last release.
#
# How it works
#
# "Released" kernel packages hold a file abi-$arch.tar that contains the
# versions of the symbols compiled in this kernel. Inside these archives
# there is a file "version", which contains the version of the ABI it
# represents.
#
# When a new release is going to be build, the packager performs a full
# build of the package to check the ABI. The %build script of the kernel
# package will leave the abi files for this specific kernel release inside
# the BUILD/kernel-$arch directory.
#
# Then, during %check, this script will be run and will check whether the
# ABIs described by SOURCES/abi-$arch.tar are *compatible* with the ones
# found in BUILD/kernel-$arch/.
#
# Note that the meaning of "ABI compatibility" here is just a set of
# acceptable changes between two given ABIs that are known to not break
# external modules in *most* of the cases, not necessarily all.
#
# As the lines from Modules.symvers are sorted. It is quite straightforward
# to find the common ABI breakers, by just running diff -u against the
# older and the newer.
#
# Output from diff -u that would make an ABI incompatible:
#
# - lines that have changed, ie. any line that begins with "-"
#
# Output from diff -u that would still make an ABI compatible:
#
# - lines that were added
#

function error(message)
{
	print message > "/dev/stderr"
}

function abort(message)
{
	error("aborting: " message)
	exit(1)
}

function abi_breaker(message)
{
	message = "ABI breaker: " message
	error(message "\n")
	error("In order to pass this check you must bump the ABI "\
	      "version in the spec file or update the ABI archive file "\
	      "(abi-ARCH.tar), by running the abi-update script or "\
              "updating your working copy.")
	if (abi_break_should_fail)
		exit(1)
}

function exists(path)
{
	if (system(sprintf("test -a '%s'", path)))
		return 0
	return 1
}

function rpm_eval(expr)
{
	cmd = sprintf("rpm --eval '%s'", expr)
	cmd | getline out
	close(cmd)
	return out
}

function basename(path)
{
	cmd = sprintf("basename '%s'", path)
	cmd | getline out
	close(cmd)
	return out
}

function abi_archive_path(mdvarch, abipath)
{
	mdvarch = rpm_eval("%mandriva_arch")
	abipath = sprintf("SOURCES/abi-%s.tar", mdvarch)
	return abipath
}

function built_dir(out)
{
	out = rpm_eval("%_arch")
	return sprintf("BUILD/kernel-%s", out)
}

function check_package_state(succ, abipath)
{
	succ = 1
	abipath = abi_archive_path()
	if (!(exists("SPECS") && exists("SOURCES"))) {
		error("This script must be run in the top directory "\
		      "of a built kernel package.") 
	      	succ = 0
	}
	else if (!exists(built_dir())) {
		error("No directory BUILD/ found, this script should be"\
		      "run only after a full kernel build (-ba)")
	    	succ = 0
	}
	else if (!exists(abipath)) {
		error(sprintf("The ABI description archive %s was "\
		              "not found", abipath))
	  	succ = 0
	}
	return succ
}

function alleged_version_path(path)
{
	path = built_dir() "/abi-alleged-version"
	return path
}

function alleged_abi_version(path, version)
{
	path = alleged_version_path()
	if (!exists(path))
		abort("file not found: " path)
	getline version < path
	return version
}

function archived_version_path(p_workdir)
{
	return p_workdir "/version"
}

function archived_abi_version(p_workdir, path, version)
{
	path = archived_version_path(p_workdir)
	getline version < path
	return version
}


function open_abi_archive(p_workdir, path, cmd, err)
{
	path = abi_archive_path()
	cmd = sprintf("tar xf '%s' -C '%s'", path, p_workdir)
	err = system(cmd)
	if (err)
		abort("failed open the abi archive: " cmd)

	path = archived_version_path(p_workdir)
	if (!exists(path))
		abort("The ABI archive was extracted, but the file "\
		      "'" path "' was not found.")
}

function compare_abi_files(p_older, p_newer, diffcmd, line, removed, added)
{
	diffcmd = sprintf("diff -U 1 '%s' '%s'", p_older, p_newer)
	added = 0
	removed = 0
	while ((diffcmd | getline line) != 0) {
		if (line ~ /^-[^-]/)
			removed++
		else if (line ~ /^+[^+]/)
			added++
	}
	close(diffcmd)

	print "lines added: " added
	print "lines removed: " removed

	if (removed) {
		# lines where removed, ABI breakage
		print
		print "ERROR: lines were changed or removed in the "\
		      "newer ABI file:\n"
		system(diffcmd)
		fflush("")
		print
		
		abi_breaker("lines were changed or removed")
	}
	else if (added) {
		print
		print "WARNING: lines were added to the ABI file:\n"
		system(diffcmd)
		fflush("")
	}
}

function find_abi_cmd(findcmd)
{
	findcmd = sprintf("find '%s' -name 'abi-*' -not -name \\*~"\
			  " -not -name abi-alleged-version", built_dir())
	return findcmd
}

function compare_abi(p_workdir, findcmd, newpath, newname, oldpath)
{
	findcmd = find_abi_cmd()
	while ((findcmd | getline newpath) != 0) {
		# for each abi file found

		newname = basename(newpath)
		if (newname == "abi-alleged-version")
			continue

		oldpath = p_workdir "/" newname
		if (!exists(oldpath)) {
			abi_breaker(sprintf("the file '%s' "\
			      "was found in the build directory, but not "\
			      "the ABI archive.", newname))
		}

		compare_abi_files(oldpath, newpath)
	}
	close(findcmd)
}

function mkdtemp(p_name, cmd, path)
{
	cmd = sprintf("mktemp -d '%s.XXXXXXXX'", p_name)
	cmd | getline path
	close(cmd)
	return path
}

function copy(p_src, p_dst, cmd)
{
	cmd = sprintf("cp '%s' '%s'", p_src, p_dst)
	system(cmd)
	fflush("")
}

function copy_abi_files(p_workdir, findcmd, path)
{
	findcmd = find_abi_cmd()
	while ((findcmd | getline path) != 0) {
		print "copying " path
		copy(path, p_workdir)
	}
	close(findcmd)
}

function copy_version_file(p_workdir, path)
{
	path = alleged_version_path()
	if (!exists(path))
		abort("The file '" path "' is missing. Can't create the "\
		      "ABI archive.")

	copy(path, archived_version_path(p_workdir))
}

function make_abi_archive(p_workdir, cmd, path)
{
	path = abi_archive_path()
	cmd = sprintf("tar cf '%s' -C '%s' .", path, p_workdir)
	system(cmd)
	fflush("")

	return path
}

function cleanup(p_path)
{
	system(sprintf("rm -rf '%s'", p_path))
}

function check_abi(workdir, newver, oldver, err)
{
	# globals
	abi_break_should_fail = 1

	err = 0

	if (!check_package_state())
		err = 1
	else {
		workdir = mkdtemp("abi-check")
		open_abi_archive(workdir)

		newver = alleged_abi_version()
		oldver = archived_abi_version(workdir)
		if (newver != oldver) {
			# the alleged and the archived abi versions aren't the
			# same, we don't have to check for abi compatibility here
			print "ABI: declared and archived versions differ, no "\
			      "need ensure ABI compatibility: "
			printf("Build version: '%s'\n", newver)
			printf("Archived version: '%s'\n", oldver)
		}
		else if (!compare_abi(workdir))
				err = 1
	}
	cleanup(workdir)

	return err
}

function update_abi(workdir, path)
{
	if (!check_package_state())
		return 0

	workdir = mkdtemp("abi-update")
	copy_abi_files(workdir)
	copy_version_file(workdir)
	path = make_abi_archive(workdir)

	print "Archive created at " path ". Possibly now you have to "\
	      "svn commit."

	cleanup(workdir)

	return 1
}

function usage()
{
	print "abi [--check | --update ]"
	print
	print "Checks or updates the ABI files on a built kernel package."
}

BEGIN {
	if (ARGV[1] == "--update")
		err = update_abi()
	else if (ARGV[1] == "--check")
		err = check_abi()
	else 
		err = usage()

	exit(err)
}
