Name: rfs
Summary:A fast remote file system, client
Version:0.10
Release:2
License: GPL
Packager: Jean-Jacques Sarton <jjsarton@users.sourceforge.net
Group: Applications/File
URL: http://www.sourceforge.net/projects/remotefs
Source: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-root
Requires: fuse-libs >= 2.6, glibc >= 2.4

%description
remotefs client


# -------------------------    prep     -----------------------------------
%prep
%setup

# -------------------------    build    -----------------------------------
%build
make rfs

# ------------------------    install    -----------------------------------
%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_prefix}/bin
cp rfs $RPM_BUILD_ROOT%{_prefix}/bin/
mkdir -p $RPM_BUILD_ROOT%{_prefix}/share/man/man1
cp man/man1/rfs.1.gz $RPM_BUILD_ROOT%{_prefix}/share/man/man1/
mkdir -p $RPM_BUILD_ROOT%{_prefix}/lib
cp librfs.so.0.11 $RPM_BUILD_ROOT%{_prefix}/lib

# ------------------------     clean     -----------------------------------
%clean
rm -rf $RPM_BUILD_ROOT

# -------------------------    files    ------------------------------------
# remember: %config, %config(missingok), %config(noreplace), %doc, %ghost,
#           %dir, %docdir, %attr(mode,user,group)
%files
%defattr(-, root, root)
#%doc homepage/* 
%attr(755, root,root) %{_prefix}/bin/rfs
%attr(611, root,root) %{_prefix}/share/man/man1/rfs.1.gz
%attr(655, root,root) %{_prefix}/lib/librfs.so.0.11

%post

# -------------------------    changelog    --------------------------------
%changelog
* Mon Aug 11 2008 Jean-Jacques Sarton <jjsarton@users.sourceforge.net>
  [0.10-1]
- Original package
