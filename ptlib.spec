%global version_major  2
%global version_minor  19
%global version_patch  4
%global version_oem    17

# Branch ID should be 0 for local builds/PRs
# Jenkins builds should use 1 for develop, 2 for master (release builds)
%{!?branch_id: %global branch_id 0}

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
%if "%{?dist}" == ".el7"
source /opt/rh/devtoolset-9/enable
%endif
%ifarch aarch64
%define arch_arg --enable-graviton2
%endif
%if %{?_with_tsan:1}%{!?_with_tsan:0}
%define tsan_arg --enable-sanitize-thread
%endif
%if %{?_with_asan:1}%{!?_with_asan:0}
%define asan_arg --enable-sanitize-address
%endif
./configure %{?arch_arg} %{?tsan_arg} %{?asan_arg} \
        --enable-cpp14 \
        --enable-exceptions \
        --with-profiling=manual \
        --disable-pthread_kill \
        --disable-vxml \
        --disable-sdl \
        --disable-sasl \
        --disable-openldap \
        --disable-plugins \
        --prefix=%{_prefix} \
        --libdir=%{_libdir} \
        PTLIB_MAJOR=%{version_major} \
        PTLIB_MINOR=%{version_minor} \
        PTLIB_PATCH=%{version_patch} \
        PTLIB_OEM=%{version_oem}
make $(pwd)/revision.h
make %{?_smp_mflags} REVISION_FILE= PTLIB_FILE_VERSION=%{version} opt debug


%install
%if "%{?dist}" == ".el7"
source /opt/rh/devtoolset-9/enable
%endif
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT PTLIB_FILE_VERSION=%{version} install
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
