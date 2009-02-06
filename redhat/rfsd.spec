Name: rfsd
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
remotefs server


# -------------------------    prep     -----------------------------------
%prep
%setup

# -------------------------    build    -----------------------------------
%build
make server

# ------------------------    install    -----------------------------------
%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_prefix}/bin
cp rfsd $RPM_BUILD_ROOT%{_prefix}/bin/
cp rfspasswd $RPM_BUILD_ROOT%{_prefix}/bin/
mkdir -p $RPM_BUILD_ROOT/etc/init.d/
cp init.d/rfsd.redhat $RPM_BUILD_ROOT/etc/init.d/rfsd
cp etc/rfs-exports $RPM_BUILD_ROOT/etc/rfs-exports
mkdir -p $RPM_BUILD_ROOT%{_prefix}/share/man/man8
cp man/man8/rfsd.8.gz $RPM_BUILD_ROOT%{_prefix}/share/man/man8/
cp man/man8/rfspasswd.8.gz $RPM_BUILD_ROOT%{_prefix}/share/man/man8/
mkdir -p $RPM_BUILD_ROOT%{_prefix}/lib

# ------------------------     clean     -----------------------------------
%clean
rm -rf $RPM_BUILD_ROOT

# -------------------------    files    ------------------------------------
# remember: %config, %config(missingok), %config(noreplace), %doc, %ghost,
#           %dir, %docdir, %attr(mode,user,group)
%files
%defattr(-, root, root)
#%doc homepage/* 
%attr(755, root,root) %{_prefix}/bin/rfsd
%attr(755, root,root) %{_prefix}/bin/rfspasswd
%attr(755, root,root) /etc/init.d/rfsd
%attr(600, root,root) /etc/rfs-exports
%attr(611, root,root) %{_prefix}/share/man/man8/rfsd.8.gz
%attr(611, root,root) %{_prefix}/share/man/man8/rfspasswd.8.gz
%config(noreplace) /etc/init.d/rfsd
%config(noreplace) /etc/rfs-exports


%post
CHKCONFIGPARM="--add rfsd"
if [ -x "/sbin/chkconfig" ]; then
    "/sbin/chkconfig" $CHKCONFIGPARM
elif [ -x "/usr/sbin/chkconfig" ]; then
    "/usr/sbin/chkconfig" $CHKCONFIGPARM
else
    echo "No chkconfig found. Chkconfig skipped."
fi

# -------------------------    changelog    --------------------------------
%changelog
* Wed Jan 21 2009 Jean-Jacques Sarton <jjsarton@users.sourceforge.net>
  [0.11-1]
- Improvments and adapted for version 0.11
* Mon Aug 11 2008 Jean-Jacques Sarton <jjsarton@users.sourceforge.net>
  [0.10-1]
- Original package
