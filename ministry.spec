Name:		ministry
Version:	0.1.3
Release:	1%{?dist}
Summary:	A statsd implementation in threaded C.

Group:		Applications/Internet
License:	Apache License 2.0 (https://www.apache.org/licenses/LICENSE-2.0.txt)
URL:		https://github.com/ghostflame/ministry
Source:		https://github.com/ghostflame/ministry/archive/%{version}.tar.gz

%description
A drop-in replacement for Etsy's statsd, written in threaded C.  Designed to
be high-performance and reasonable to work with.

%prep
%setup -q



%build
make %{?_smp_mflags}


%install
make install DESTDIR=%{buildroot}


%files
%doc



%changelog

