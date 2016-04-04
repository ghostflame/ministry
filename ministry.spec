Name:		ministry
Version:	0.2.15
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
# make the user we want
getent group  ministry > /dev/null || groupadd -r ministry
getent passwd ministry > /dev/null || useradd  -r -g ministry -M -d /etc/ministry -s /sbin/nologin -c 'Minister for Statistics' ministry
# note, we do not remove the group/user post uninstall
# see https://fedoraproject.org/wiki/Packaging:UsersAndGroups for reasoning
mkdir -p /var/log/ministry
chown ministry:ministry /var/log/ministry



%install
DESTDIR=%{buildroot} \
BINDIR=%{buildroot}%{_bindir} \
LRTDIR=%{buildroot}/etc/logrotate.d \
CFGDIR=%{buildroot}/etc/ministry \
LOGDIR=%{buildroot}/var/log/ministry \
DOCDIR=%{buildroot}%{_docdir}/ministry \
MANDIR=%{buildroot}%{_mandir} \
UNIDIR=%{buildroot}%{_unitdir} \
USER=ministry GROUP=ministry \
make unitinstall


%files
%doc %{_docdir}/ministry/
%config(noreplace) /etc/ministry/ministry.conf
%config(noreplace) /etc/logrotate.d/ministry
%{_bindir}/ministry
%{_mandir}/man1/ministry.1.gz
%{_mandir}/man5/ministry.conf.5.gz
%{_unitdir}/ministry.service

%changelog

