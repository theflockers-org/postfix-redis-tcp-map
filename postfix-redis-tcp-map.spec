Name: postfix-redis-tcp-map
Version:    0.1
Release:	1%{?dist}
Summary:	Postfix Redis tcp lookup map

Group:		Server
License:	LGPL
URL:		
Source0:	

BuildRequires:	
Requires:	

%description


%prep
%setup -q


%build
%configure
make %{?_smp_mflags}


%install
%make_install


%files
%doc



%changelog

