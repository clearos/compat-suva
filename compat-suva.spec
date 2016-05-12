%define findutils_version 4.5.11

Name: compat-suva
Version: 1.0
Release: 13%{dist}
Vendor: ClearFoundation
License: GPL
Group: Application/Misc
Packager: ClearFoundation
Requires: suva-client >= 2.0.0
Provides: suvlets
Obsoletes: suvlets, suva-client < 2.0.0
BuildRequires: bison
BuildRequires: flex
BuildRequires: zlib-devel
BuildRequires: glibc-static
Requires: security-audit-aide
Requires(pre): /sbin/ldconfig
Source0: %{name}-%{version}.tar.gz
BuildRoot: /var/tmp/%{name}-%{version}
Summary: Default suvlets for managed services

%description
Default suvlets for managed services
Report bugs to: http://www.clearfoundation.com/docs/developer/bug_tracker/


###############################################################################
#
# Build
#
###############################################################################

%prep
%setup -q
./autogen.sh
%{configure} --without-selinux

%build
make %{?_smp_mflags}

###############################################################################
#
# Install
#
###############################################################################

%install

# This installs the following default services:
# Blank
# BWMeter
# DeviceInfo
# SecurityAudit
# Snort
# Software

mkdir -p -m 755 $RPM_BUILD_ROOT/var/lib/suvlets
mkdir -p -m 755 $RPM_BUILD_ROOT/var/lib/suvlets/Blank
mkdir -p -m 755 $RPM_BUILD_ROOT/var/lib/suvlets/BWMeter
mkdir -p -m 755 $RPM_BUILD_ROOT/var/lib/suvlets/DeviceInfo
mkdir -p -m 755 $RPM_BUILD_ROOT/var/lib/suvlets/SecurityAudit/db
mkdir -p -m 755 $RPM_BUILD_ROOT/var/lib/suvlets/Snort
mkdir -p -m 755 $RPM_BUILD_ROOT/var/lib/var/tmp/Snort
mkdir -p -m 755 $RPM_BUILD_ROOT/var/lib/suvlets/Software

# Binaries
#---------
cp suvlets/Blank/remote/Blank $RPM_BUILD_ROOT/var/lib/suvlets/Blank/
cp suvlets/BWMeter/remote/BWMeter $RPM_BUILD_ROOT/var/lib/suvlets/BWMeter/
cp suvlets/DeviceInfo/remote/DeviceInfo $RPM_BUILD_ROOT/var/lib/suvlets/DeviceInfo/
cp suvlets/Snort/remote/Snort $RPM_BUILD_ROOT/var/lib/suvlets/Snort/
cp suvlets/Software/remote/Software $RPM_BUILD_ROOT/var/lib/suvlets/Software/
cp suvlets/Software/remote/software-wrapper $RPM_BUILD_ROOT/var/lib/suvlets/Software/


# Security Audit
#---------------
cp -a suvlets/SecurityAudit/remote/SecurityAudit $RPM_BUILD_ROOT/var/lib/suvlets/SecurityAudit/
cp -a suvlets/SecurityAudit/remote/findutils-%{findutils_version}/find/find $RPM_BUILD_ROOT/var/lib/suvlets/SecurityAudit/
cp -a suvlets/SecurityAudit/remote/utils/dotfiles $RPM_BUILD_ROOT/var/lib/suvlets/SecurityAudit/
cp -a suvlets/SecurityAudit/remote/utils/passwordless $RPM_BUILD_ROOT/var/lib/suvlets/SecurityAudit/
cp -a suvlets/SecurityAudit/remote/utils/xuids $RPM_BUILD_ROOT/var/lib/suvlets/SecurityAudit/
touch $RPM_BUILD_ROOT/var/lib/suvlets/SecurityAudit/db/aide.db
touch $RPM_BUILD_ROOT/var/lib/suvlets/SecurityAudit/db/dotfiles.db
touch $RPM_BUILD_ROOT/var/lib/suvlets/SecurityAudit/db/xuids.db


###############################################################################
#
# Pre/Post scripts
#
###############################################################################

%post
# TODO: Suva/2 currently has no way of adding suvlets without parsing the text
# configuration file directly.  Need to implement similar functionality as
# Suva/2 suvactl --add-suvlet using Berkeley DB.  For the time being, we ship
# a preconfigured suvad.conf with the following entries...

#if [ "$1" = "1" ]; then
#	/var/lib/bin/suvactl --add-org pointclark.net 2>/dev/null
#fi
#/usr/local/suva/bin/suvactl --add-suvlet Blank -o pointclark.net --cmd Blank 2>/dev/null
#/usr/local/suva/bin/suvactl --add-suvlet BWMeter -o pointclark.net --cmd BWMeter 2>/dev/null
#/usr/local/suva/bin/suvactl --add-suvlet DeviceInfo -o pointclark.net --cmd DeviceInfo  2>/dev/null
#/usr/local/suva/bin/suvactl --add-suvlet SecurityAudit -o pointclark.net --cmd SecurityAudit 2>/dev/null
#/usr/local/suva/bin/suvactl --add-suvlet Snort -o pointclark.net --cmd Snort 2>/dev/null
#/usr/local/suva/bin/suvactl --add-suvlet Software -o pointclark.net --cmd Software 2>/dev/null

# XXX: Hack to force a clean from legacy suvlets RPM un-install,
# as suvactl (see postun below) no longer exists.  Here we're removing the fake suvactl:
if [ -f /usr/local/suva/bin/suvactl ]; then
	rm -f /usr/local/suva/bin/suvactl
fi

%postun
#if [ "$1" = "0" ]; then
#	/usr/local/suva/bin/suvactl --delete-suvlet Blank -o pointclark.net 2>/dev/null
#	/usr/local/suva/bin/suvactl --delete-suvlet BWMeter -o pointclark.net 2>/dev/null
#	/usr/local/suva/bin/suvactl --delete-suvlet DeviceInfo -o pointclark.net 2>/dev/null
#	/usr/local/suva/bin/suvactl --delete-suvlet SecurityAudit -o pointclark.net 2>/dev/null
#	/usr/local/suva/bin/suvactl --delete-suvlet Snort -o pointclark.net 2>/dev/null
#	/usr/local/suva/bin/suvactl --delete-suvlet Software -o pointclark.net 2>/dev/null
#fi


%clean
rm -rf $RPM_BUILD_ROOT


###############################################################################
#
# File list
#
###############################################################################

%files
%attr(0750,suva,suva) %dir /var/lib/suvlets/
%attr(0750,suva,suva) %dir /var/lib/suvlets/Blank/
%attr(0750,suva,suva) %dir /var/lib/suvlets/BWMeter/
%attr(0750,suva,suva) %dir /var/lib/suvlets/DeviceInfo/
%attr(0750,suva,suva) %dir /var/lib/suvlets/Snort/
%attr(0750,suva,suva) %dir /var/lib/suvlets/Software/
%attr(0750,suva,suva) /var/lib/suvlets/Blank/Blank
%attr(0750,suva,suva) /var/lib/suvlets/BWMeter/BWMeter
%attr(0750,suva,suva) /var/lib/suvlets/DeviceInfo/DeviceInfo
%attr(0750,suva,suva) /var/lib/suvlets/Software/Software

%attr(0755,suva,suva) %dir /var/lib/var/tmp/Snort/
%attr(0750,suva,suva) /var/lib/suvlets/Snort/Snort

# Wrappers and scripts
%attr(4750,root,suva) /var/lib/suvlets/Software/software-wrapper

# Security audit
%attr(0750,suva,suva) %dir /var/lib/suvlets/SecurityAudit
%attr(0750,suva,suva) %dir /var/lib/suvlets/SecurityAudit/db
%attr(0750,suva,suva) /var/lib/suvlets/SecurityAudit/SecurityAudit
%attr(4750,root,suva) /var/lib/suvlets/SecurityAudit/find
%attr(4750,root,suva) /var/lib/suvlets/SecurityAudit/dotfiles
%attr(4750,root,suva) /var/lib/suvlets/SecurityAudit/passwordless
%attr(4750,root,suva) /var/lib/suvlets/SecurityAudit/xuids
%config(noreplace) %attr(0660,root,suva) /var/lib/suvlets/SecurityAudit/db/aide.db
%config(noreplace) %attr(0660,root,suva) /var/lib/suvlets/SecurityAudit/db/dotfiles.db
%config(noreplace) %attr(0660,root,suva) /var/lib/suvlets/SecurityAudit/db/xuids.db

