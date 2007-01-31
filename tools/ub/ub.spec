Name: ub
Summary: unionfs brancher
Version: 0.01
Release: 1
License: Apache License
Group: Development/Other
BuildArch: noarch
BuildRoot: %{_tmppath}/%{name}
Url: http://bogdano.freezope.org/Tech/UnionfsBrancher

%description
ub manages "branches" of unionfs chroots. It tries to ease the task of
creating chroots in order to perform small tests.

Requires:

- Smart package manager (labix.org/smart),
- gmake, and
- unionfs support.

%prep 

%install
install -m 755 ub %buildroot/%_sbindir
install -m 755 sub %buildroot/%_bindir
install -m 755 conveyor-chroot-builder2 %buildroot/%_bindir

%clean
rm -rf $RPM_BUILD_ROOT

%files 
%defattr(-,root,root)
%_sbindir/ub
%_bindir/sub
%_bindir/conveyor-chroot-builder2
