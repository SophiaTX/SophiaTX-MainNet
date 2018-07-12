Name:           sophiatx 
Version:        1.0
Release:        0
License:        MIT
Source:         %{name}.tar.gz
Summary:        Official SophiaTX deamon
Url:            https://www.sophiatx.com/
Vendor:         Equidato Technologies AG
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
            
%description
Official SophiaTX deamon

%prep
%setup -c %{name}

%install
mkdir -p $RPM_BUILD_ROOT/usr/local/bin
install -m 700 sophiatxd $RPM_BUILD_ROOT/usr/local/bin

%files
%defattr(-,root,root)
/usr/local/bin/sophiatxd

%clean
rm -rf $RPM_BUILD_ROOT
            
%changelog
* Thu Jul 12 2018  Matus Kysel <matus.kysel@sophiatx.com>
- 1.0 r1 First release
