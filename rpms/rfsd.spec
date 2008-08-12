Name: rfsd
Summary:A fast remote file system, server
Version:0.10
Release:2P
License: GPL
Packager: Jean-Jacques Sarton <jjsarton@users.sourceforge.net
Group: System Environment/Daemons
URL: http://www.sourceforge.net/projects/remotefs
Source: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-root


%description
rfsd is the server for the fast remote file system remotefs


# -------------------------    prep     -----------------------------------
%prep
%setup

# -------------------------    build    -----------------------------------
%build
make rfsd
make rfspasswd

# ------------------------    install    -----------------------------------
%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_prefix}/bin
cp rfsd $RPM_BUILD_ROOT%{_prefix}/bin/
cp rfspasswd $RPM_BUILD_ROOT%{_prefix}/bin/
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d/
cp init.d/rfsd.redhat $RPM_BUILD_ROOT/etc/rc.d/init.d/rfsd

# ------------------------     clean     -----------------------------------
%clean
rm -rf $RPM_BUILD_ROOT

# -------------------------    files    ------------------------------------
# remember: %config, %config(missingok), %config(noreplace), %doc, %ghost,
#           %dir, %docdir, %attr(mode,user,group)
%files
%defattr(-, root, root)
%doc homepage/* 
%attr(755, root,root) %{_prefix}/bin/rfsd
%attr(755, root,root) %{_prefix}/bin/rfspasswd
%attr(755, root,root) /etc/rc.d/init.d/rfsd

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
* Mon Aug 11 2008 Jean-Jacques Sarton <jjsarton@users.sourceforge.net>
  [0.10-2P]
- Original package
