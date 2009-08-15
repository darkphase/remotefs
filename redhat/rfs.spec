Name: rfs
Summary:remote file system, client
Version:
Release:2
License: GPL
Packager: Jean-Jacques Sarton <jjsarton@users.sourceforge.net
Group: Applications/File
URL: http://www.sourceforge.net/projects/remotefs
Source: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-root
Requires: %__find_requires
Provides: %__find_provides


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
cp librfs.so.%{version} $RPM_BUILD_ROOT%{_prefix}/lib
ln -s librfs.so.%{version} $RPM_BUILD_ROOT%{_prefix}/lib/librfs.so

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
%attr(655, root,root) %{_prefix}/lib/librfs.so.%{version}
%attr(777, root,root) %{_prefix}/lib/librfs.so

%post

# -------------------------    changelog    --------------------------------
%changelog
* Wed Jan 21 2009 Jean-Jacques Sarton <jjsarton@users.sourceforge.net>
  [0.11-1]
- Improvments and adapted for version 0.11
* Mon Aug 11 2008 Jean-Jacques Sarton <jjsarton@users.sourceforge.net>
  [0.10-1]
- Original package
