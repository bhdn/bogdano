
bin_SCRIPTS = sub conveyor-chroot-builder2
sbin_SCRIPTS = ub

pkgdata_DATA = chroot-builder.conf common-hooks.conf

EXTRA_DIST = sub \
	     ub \
	     conveyor-chroot-builder2 \
	     ub.spec \
	     chroot-builder.conf \
	     common-hooks.conf

specfile = ub.spec

rpm_topdir = $(CURDIR)/build/
rpm_buildroot = $(rpm_topdir)BUILDROOT/
rpm_specsdir = $(rpm_topdir)SPECS/
rpm_builddir = $(rpm_topdir)BUILD/
rpm_srpmsdir = $(rpm_topdir)SRPMS/
rpm_sourcesdir = $(rpm_topdir)SOURCES/
rpm_rpmsdir = $(rpm_topdir)RPMS/

rpm_dirs = $(rpm_topdir) $(rpm_specsdir) $(rpm_sourcesdir) \
	   $(rpm_rpmsdir) $(rpm_builddir) $(rpm_srpmsdir) \
	   $(rpm_buildroot)
$(rpm_dirs):
	mkdir -p $@

rpm: dist-gzip $(rpm_dirs)
	cp $(distdir).tar.gz $(rpm_sourcesdir)
	cp $(specfile) $(rpm_specsdir)
	rpm -ba --define "_topdir $(rpm_topdir)" $(rpm_specsdir)$(specfile)
