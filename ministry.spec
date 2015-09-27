Name:		ministry
Version:	0.1.0
Release:	1%{?dist}
Summary:	A statsd implementation in threaded C.

Group:		Applications/Internet
License:	Apache License 2.0 (https://www.apache.org/licenses/LICENSE-2.0.txt)
URL:		https://github.com/ghostflame/ministry
Source0:	https://github.com/ghostflame/ministry/archive/master.zip

BuildRequires:
Requires:	

%description
A drop-in replacement for Etsy's statsd, written in threaded C.  Designed to
be high-performance and reasonable to work with.

%prep
%setup -q
unzip master.zip
cd ministry-master



%build
make %{?_smp_mflags}


%install
make install DESTDIR=%{buildroot}


%files
%doc



%changelog

