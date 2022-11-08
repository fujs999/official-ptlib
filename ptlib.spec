%global version_major  2
%global version_minor  19
%global version_patch  4
%global version_oem    18

# Branch ID should be 0 for local builds/PRs
# Jenkins builds should use 1 for develop, 2 for master (release builds)
%{!?branch_id:     %global branch_id     0}
%{!?version_stage: %global version_stage AlphaCode}

# Disable the separate debug package and automatic stripping, as detailed here:
# http://fedoraproject.org/wiki/How_to_create_an_RPM_package
%global _enable_debug_package 0
%global debug_package %{nil}
%global __os_install_post /usr/lib/rpm/brp-compress %{nil}

Name:           collab-ptlib
Version:        %{version_major}.%{version_minor}.%{version_patch}.%{version_oem}
Release:        %{branch_id}%{?jenkins_release}%{?dist}
Summary:        PTLib: Portable Tools Library

Group:          System Environment/Libraries
License:        MPL 1.0 + others
URL:            https://github.com/Class-Collab/rpm-ptlib
Source0:        source.tgz

# Initial dependency list based on Opal wiki:
# http://wiki.opalvoip.org/index.php?n=Main.BuildingPTLibUnix
# Optional build dependencies not needed for the MCU are commented-out
BuildRequires:  %__sed

BuildRequires:  which
BuildRequires:  gperftools
BuildRequires:  openssl-devel
BuildRequires:  expat-devel
BuildRequires:  ImageMagick-devel

%description
PTLib: Portable Tools Library

%package        devel
Summary:        Development files for %{name}
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}

Requires:       openssl-devel
Requires:       ImageMagick-devel

Requires:       expat-devel
Requires:       libpcap-devel
Requires:       ncurses-devel

%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.

%package        static
Summary:        Development files for %{name}
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}
Requires:       %{name}-devel = %{version}-%{release}

%description    static
The %{name}-static package contains static libraries for
developing applications that use %{name}.


%prep
%setup -q -c -n ptlib

%build
%if 0%{?el7}
source /opt/rh/devtoolset-9/enable
%endif
sed -i \
    -e "s/MAJOR_VERSION.*/MAJOR_VERSION %{version_major}/" \
    -e "s/MINOR_VERSION.*/MINOR_VERSION %{version_minor}/" \
    -e "s/BUILD_TYPE.*/BUILD_TYPE %{version_stage}/"       \
    -e "s/PATCH_VERSION.*/PATCH_VERSION %{version_patch}/" \
    -e "s/OEM_VERSION.*/OEM_VERSION %{version_oem}/"       \
    version.h
%configure \
%ifarch aarch64
    --enable-graviton2 \
%endif
%if 0%{?_with_tsan:1}
    --enable-sanitize-thread \
%endif
%if 0%{?_with_asan:1}
    --enable-sanitize-address \
%endif
    --enable-cpp14 \
    --enable-exceptions \
    --with-profiling=manual \
    --disable-pthread_kill \
    --disable-vxml \
    --disable-sdl \
    --disable-sasl \
    --disable-openldap \
    --disable-plugins
%make_build opt debug


%install
%if 0%{?el7}
source /opt/rh/devtoolset-9/enable
%endif
rm -rf $RPM_BUILD_ROOT
%make_install
find $RPM_BUILD_ROOT -name '*.la' -exec rm -f {} ';'


%clean
rm -rf $RPM_BUILD_ROOT


%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%doc ReadMe.txt History.txt
%{_libdir}/*.so.*

%files devel
%defattr(-,root,root,-)
%{_includedir}/*
%{_libdir}/*.so
%{_libdir}/pkgconfig/*.pc
%{_datadir}/ptlib

%files static
%defattr(-,root,root,-)
%{_libdir}/*.a


%changelog
* Mon Nov 21 2016 Gavin Llewellyn <gavin.llewellyn@blackboard.com> - 2.17.1-1
- Initial RPM release
