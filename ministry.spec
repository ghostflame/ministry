Name:		ministry
Version:	0.7.0
Release:	1%{?dist}
Summary:	A statsd implementation in threaded C.

Group:		Applications/Internet
License:	ASL 2.0
URL:		https://github.com/ghostflame/ministry
Source:		https://github.com/ghostflame/ministry/archive/%{version}.tar.gz

BuildRequires: gcc libcurl-devel libmicrohttpd-devel openssl-devel json-c-devel gnutls-devel
Requires(pre): shadow-utils systemd libcurl libmicrohttpd openssl json-c gnutls

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
mkdir -p /var/log/ministry /var/log/ministry/carbon-copy /var/log/ministry/metric-filter
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

%post
chown -R ministry:ministry /etc/ministry/ssl


%files
%doc %{_docdir}/ministry/
%config(noreplace) /etc/ministry/ministry.conf
%config(noreplace) /etc/ministry/carbon-copy.conf
%config(noreplace) /etc/ministry/ministry-test.conf
%config(noreplace) /etc/ministry/metric-filter.conf
%config(noreplace) /etc/ministry/archivist.conf
%config(noreplace) /etc/logrotate.d/ministry
%config(noreplace) /etc/logrotate.d/carbon-copy
%config(noreplace) /etc/logrotate.d/metric-filter
%config(noreplace) /etc/logrotate.d/archivist
%config(noreplace) /etc/ministry/ssl/cert.pem
%config(noreplace) /etc/ministry/ssl/key.pem
%{_bindir}/ministry
%{_bindir}/carbon-copy
%{_bindir}/ministry-test
%{_bindir}/metric-filter
%{_bindir}/archivist
%{_mandir}/man1/ministry.1.gz
%{_mandir}/man1/ministry-test.1.gz
%{_mandir}/man1/carbon-copy.1.gz
%{_mandir}/man1/metric-filter.1.gz
%{_mandir}/man1/archivist.1.gz
%{_mandir}/man5/ministry.conf.5.gz
%{_mandir}/man5/ministry-test.conf.5.gz
%{_mandir}/man5/carbon-copy.conf.5.gz
%{_mandir}/man5/metric-filter.conf.5.gz
%{_mandir}/man5/archivist.conf.5.gz
%{_unitdir}/ministry.service
%{_unitdir}/carbon-copy.service
%{_unitdir}/metric-filter.service
%{_unitdir}/archivist.service

%changelog

