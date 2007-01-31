Name: ub
Summary: unionfs brancher
Version: 0.01
Release: 1
Source: http://bogdano.googlecode.com/files/%name-%version.tar.gz
License: Apache License
Group: Development/Other
Requires: make
Requires: /usr/sbin/unionctl
BuildArch: noarch
BuildRoot: %{_tmppath}/%{name}
Url: http://bogdano.freezope.org/Tech/UnionfsBrancher

%description
ub manages "branches" of unionfs chroots. It tries to ease the task of
creating chroots in order to perform small tests.

Requires:

- Smart package manager (optional, at labix.org/smart),
- gmake, and
- unionfs support.

%prep 
%setup -q

%build
%configure
%make

%install
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%files 
%defattr(-,root,root)
%_sbindir/ub
%_bindir/sub
%_bindir/conveyor-chroot-builder2
%_datadir/%name/chroot-builder.conf
%_datadir/%name/common-hooks.conf
