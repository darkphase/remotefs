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
Requires: glibc >= 2.4

%description
remotefs server


# -------------------------    prep     -----------------------------------
%prep
%setup

# -------------------------    build    -----------------------------------
%build
make server
#make -f Makefiles/base.mk clean_build
#make rfspasswd

# ------------------------    install    -----------------------------------
%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_prefix}/bin
cp rfsd $RPM_BUILD_ROOT%{_prefix}/bin/
cp rfspasswd $RPM_BUILD_ROOT%{_prefix}/bin/
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d/
cp init.d/rfsd.redhat $RPM_BUILD_ROOT/etc/rc.d/init.d/rfsd
cp etc/rfs-exports $RPM_BUILD_ROOT/etc/rfs-exports
mkdir -p $RPM_BUILD_ROOT%{_prefix}/share/man/man8
cp man/man8/rfsd.8.gz $RPM_BUILD_ROOT%{_prefix}/share/man/man8/
cp man/man8/rfspasswd.8.gz $RPM_BUILD_ROOT%{_prefix}/share/man/man8/
mkdir -p $RPM_BUILD_ROOT%{_prefix}/lib
cp librfsd.so.0.11 $RPM_BUILD_ROOT%{_prefix}/lib

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
%attr(755, root,root) /etc/rc.d/init.d/rfsd
%attr(600, root,root) /etc/rfs-exports
%attr(611, root,root) %{_prefix}/share/man/man8/rfsd.8.gz
%attr(611, root,root) %{_prefix}/share/man/man8/rfspasswd.8.gz
%attr(655, root,root) %{_prefix}/lib/librfsd.so.0.11

%pre
if [ -f /etc/rfs-exports ]; then
    mv /etc/rfs-exports /etc/rfs-exports-pre-rpm
fi

%post
CHKCONFIGPARM="--add rfsd"
if [ -x "/sbin/chkconfig" ]; then
    "/sbin/chkconfig" $CHKCONFIGPARM
elif [ -x "/usr/sbin/chkconfig" ]; then
    "/usr/sbin/chkconfig" $CHKCONFIGPARM
else
    echo "No chkconfig found. Chkconfig skipped."
fi
if [ -f /etc/rfs-exports-pre-rpm ]; then
    mv /etc/rfs-exports /etc/rfs-exports.newrpm
    mv /etc/rfs-exports-pre-rpm /etc/rfs-exports
fi

%preun
if [ -f /etc/rfs-exports.newrpm ]; then
    mv /etc/rfs-exports /etc/rfs-exports-old
    mv /etc/rfs-exports.newrpm /etc/rfs-exports
fi

%postun
if [ -f  etc/rfs-exports-old ]; then
    mv /etc/rfs-exports-old /etc/rfs-exports
fi

# -------------------------    changelog    --------------------------------
%changelog
* Mon Aug 11 2008 Jean-Jacques Sarton <jjsarton@users.sourceforge.net>
  [0.10-1]
- Original package
