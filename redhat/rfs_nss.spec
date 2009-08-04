Name: rfs_nss
Summary:A fast remote file system, server
Version:0.10
Release:1
License: GPL
Packager: Jean-Jacques Sarton <jjsarton@users.sourceforge.net
Group: System Environment/Daemons
URL: http://www.sourceforge.net/projects/remotefs
Source: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-root
Requires: %__find_requires
Provides: %__find_provides

%description
remotefs nss component


# -------------------------    prep     -----------------------------------
%prep
%setup

# -------------------------    build    -----------------------------------
%build
make librfs  libnss nss

# ------------------------    install    -----------------------------------
%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_prefix}/bin
cp rfs_nssd $RPM_BUILD_ROOT%{_prefix}/bin/
mkdir -p $RPM_BUILD_ROOT%{_prefix}/lib
cp libnss_rfs.so.2 $RPM_BUILD_ROOT%{_prefix}/lib
mkdir -p $RPM_BUILD_ROOT%{_prefix}/share/man/man1
cp man/man1/rfs_nssd.1.gz $RPM_BUILD_ROOT%{_prefix}/share/man/man1/

# ------------------------     clean     -----------------------------------
%clean
rm -rf $RPM_BUILD_ROOT

# -------------------------    files    ------------------------------------
# remember: %config, %config(missingok), %config(noreplace), %doc, %ghost,
#           %dir, %docdir, %attr(mode,user,group)
%files
%defattr(-, root, root)
#%doc homepage/* 
%attr(755, root,root) %{_prefix}/bin/rfs_nssd
%attr(755, root,root) %{_prefix}/lib/libnss_rfs.so.2
%attr(644, root,root) %{_prefix}/share/man/man1/rfs_nssd.1.gz

%post

# -------------------------    changelog    --------------------------------
%changelog
* Mon Aug 04 2009 Jean-Jacques Sarton <jjsarton@users.sourceforge.net>
  [0.12-1]
- Original package
