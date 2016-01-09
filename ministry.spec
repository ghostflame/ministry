Name:		ministry
Version:	0.2.0
Release:	1%{?dist}
Summary:	A statsd implementation in threaded C.

Group:		Applications/Internet
License:	ASL 2.0
URL:		https://github.com/ghostflame/ministry
Source:		https://github.com/ghostflame/ministry/archive/%{version}.tar.gz

BuildRequires: gcc
Requires(pre): shadow-utils systemd

%description
A drop-in replacement for Etsy's statsd, written in threaded C.  Designed to
be high-performance and reasonable to work with.

%prep
%setup -q

%build
make %{?_smp_mflags}

%pre
mkdir %{_docdir}/ministry
mkdir %{buildroot}/var/log/ministry
mkdir %{buildroot}/etc/ministry
getent group  ministry > /dev/null || groupadd -r ministry
getent passwd ministry > /dev/null || useradd  -r -g ministry -d %{buildroot}/var/log/ministry -s /sbin/nologin -c 'Minister for Statistics' ministry
chown ministry:ministry %{buildroot}/var/log/ministry
# note, we do not remove the group/user post uninstall
# see https://fedoraproject.org/wiki/Packaging:UsersAndGroups for reasoning


%install
DESTDIR=%{buildroot} BINDIR=%{_bindir} CFGDIR=%{buildroot}/etc/ministry LOGDIR=%{buildroot}/var/log/ministry DOCDIR=%{_docdir}/ministry MANDIR=%{_mandir} UNIDIR=%{_unitdir} make install

%postun
# tidy up logs
rm -rf /var/log/ministry


%files
%doc
/etc/ministry/
/etc/logrotate.d/ministry
%{_bindir}/ministry
%{_mandir}/man1/ministry.1.gz
%{_mandir}/man5/ministry.conf.5.gz
%{_docdir}/ministry/
%{_unitdir}/ministry.service

%changelog

