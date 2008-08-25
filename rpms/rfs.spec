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


%description
rfs is the client for the fast remote file system remotefs


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

# ------------------------     clean     -----------------------------------
%clean
rm -rf $RPM_BUILD_ROOT

# -------------------------    files    ------------------------------------
# remember: %config, %config(missingok), %config(noreplace), %doc, %ghost,
#           %dir, %docdir, %attr(mode,user,group)
%files
%defattr(-, root, root)
%doc homepage/* 
%attr(755, root,root) %{_prefix}/bin/rfs

%post

# -------------------------    changelog    --------------------------------
%changelog
* Mon Aug 11 2008 Jean-Jacques Sarton <jjsarton@users.sourceforge.net>
  [0.10-2P]
- Original package
