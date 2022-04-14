Name: postfix-redis-tcp-map
Version:    0.1
Release:	1%{?dist}
Summary:	Postfix Redis tcp lookup table

Group:		Server
License:	LGPL-3.0
Source0:	postfix-redis-tcp-map-%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildArch:  x86_64

BuildRequires:  glib2-devel
BuildRequires:  libevent-devel
BuildRequires:  hiredis-devel
BuildRequires:  mysql-devel
BuildRequires:	libpq-devel
BuildRequires:	openldap-devel

Requires: glib2
Requires: libevent
Requires: hiredis
Requires: mysql
Requires: libpq
Requires: openldap

%description
Postfix Redis tcp map is a implementation of Postfix's tcp lookup table.

%prep
%setup -c -n %{name}-%{version}

%build
make %{?_smp_mflags}

%install
%{__rm} -fr %{buildroot}
%{__mkdir} -p %{buildroot}/usr/sbin
%{__mkdir} -p %{buildroot}/etc/postfix-redis/
%{__mkdir} -p %{buildroot}/usr/lib/systemd/system/
%{__install} -Dp -m0755 %{_builddir}/%{name}-%{version}/postfix-redis \
    %{buildroot}/usr/sbin
%{__install} -Dp -m0644 %{_builddir}/%{name}-%{version}/postfix-redis.cfg.sample \
    %{buildroot}/etc/postfix-redis/
%{__install} -Dp -m0644 %{_builddir}/%{name}-%{version}/postfix-redis.service \
    %{buildroot}/usr/lib/systemd/system/

%post
%systemd_post %{pkgname}.service

%preun
%systemd_preun %{pkgname}.service

%postun
%systemd_postun_with_restart  %{pkgname}.service

%files
%defattr(-,root,root,-)
%{_sbindir}/postfix-redis
%{_sysconfdir}/postfix-redis/postfix-redis.cfg.sample
%{_unitdir}/postfix-redis.service

%changelog
* Thu Apr 14 2022 Leandro Mendes <theflockers@gmail.com> 0.1
- Initial release
