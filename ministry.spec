Name:		ministry
Version:	0.4.5
Release:	1%{?dist}
Summary:	A statsd implementation in threaded C.

Group:		Applications/Internet
License:	ASL 2.0
URL:		https://github.com/ghostflame/ministry
Source:		https://github.com/ghostflame/ministry/archive/%{version}.tar.gz

BuildRequires: gcc libcurl-devel libmicrohttpd-devel openssl-devel
Requires(pre): shadow-utils systemd libcurl libmicrohttpd openssl

%description
A drop-in replacement for Etsy's statsd, written in threaded C.  Designed to
be high-performance and reasonable to work with.  Comes with a test/load-gen
program, ministry-test, and a metrics router, carbon-copy, for load-balancing.

%prep
%setup -q

%build
make %{?_smp_mflags}

%pre
# make the user we want
getent group  ministry > /dev/null || groupadd -r ministry
getent passwd ministry > /dev/null || useradd  -r -g ministry -M -d /etc/ministry -s /sbin/nologin -c 'Minister for Statistics' ministry
# note, we do not remove the group/user post uninstall
# see https://fedoraproject.org/wiki/Packaging:UsersAndGroups for reasoning
mkdir -p -m 700 /etc/ministry/ssl
chown ministry:ministry /etc/ministry/ssl
mkdir -p /var/log/ministry
chown -R ministry:ministry /var/log/ministry


%install
DESTDIR=%{buildroot} \
BINDIR=%{buildroot}%{_bindir} \
LRTDIR=%{buildroot}/etc/logrotate.d \
CFGDIR=%{buildroot}/etc/ministry \
LOGDIR=%{buildroot}/var/log/ministry \
DOCDIR=%{buildroot}%{_docdir}/ministry \
MANDIR=%{buildroot}%{_mandir} \
UNIDIR=%{buildroot}%{_unitdir} \
SSLDIR=%{buildroot}/etc/ministry/ssl \
USER=ministry GROUP=ministry \
make unitinstall


%files
%doc %{_docdir}/ministry/
%config(noreplace) /etc/ministry/ministry.conf
%config(noreplace) /etc/ministry/carbon-copy.conf
%config(noreplace) /etc/ministry/ministry-test.conf
%config(noreplace) /etc/logrotate.d/ministry
%config(noreplace) /etc/logrotate.d/carbon-copy
%config(noreplace) /etc/ministry/ssl/cert.pem
%config(noreplace) /etc/ministry/ssl/key.pem
%{_bindir}/ministry
%{_bindir}/carbon-copy
%{_bindir}/ministry-test
%{_mandir}/man1/ministry.1.gz
%{_mandir}/man1/ministry-test.1.gz
%{_mandir}/man1/carbon-copy.1.gz
%{_mandir}/man5/ministry.conf.5.gz
%{_unitdir}/ministry.service
%{_unitdir}/carbon-copy.service

%changelog

