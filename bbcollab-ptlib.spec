%global version_major  2
%global version_minor  19
%global version_patch  4
%global version_oem    16

# Branch ID should be 0 for local builds/PRs
# Jenkins builds should use 1 for develop, 2 for master (release builds)
%{!?branch_id: %global branch_id 0}

# Disable the separate debug package and automatic stripping, as detailed here:
# http://fedoraproject.org/wiki/How_to_create_an_RPM_package
%global _enable_debug_package 0
%global debug_package %{nil}
%global __os_install_post /usr/lib/rpm/brp-compress %{nil}

Name:           bbcollab-ptlib
Version:        %{version_major}.%{version_minor}.%{version_patch}.%{version_oem}
Release:        %{branch_id}%{?jenkins_release}%{?dist}
Summary:        PTLib: Portable Tools Library

Group:          System Environment/Libraries
License:        MPL 1.0 + others
URL:            https://stash.bbpd.io/projects/COL/repos/zsdk-ptlib/browse
Source0:        zsdk-ptlib.src.tgz

# Initial dependency list based on Opal wiki:
# http://wiki.opalvoip.org/index.php?n=Main.BuildingPTLibUnix
# Optional build dependencies not needed for the MCU are commented-out
BuildRequires:  %__sed

BuildRequires:  devtoolset-9-gcc-c++
BuildRequires:  openssl-devel
BuildRequires:  ImageMagick-devel

#BuildRequires:  cyrus-sasl-devel
BuildRequires:  expat-devel
BuildRequires:  gperftools
# gstreamer 0.10 support seems to be broken, but 1.0 isn't available
#BuildRequires:  gstreamer-plugins-base-devel
#BuildRequires:  gstreamer-devel
#BuildRequires:  libjpeg-devel
BuildRequires:  libpcap-devel
#BuildRequires:  lua-devel
BuildRequires:  ncurses-devel
#BuildRequires:  openldap-devel
#BuildRequires:  SDL-devel
#BuildRequires:  unixODBC-devel
#BuildRequires:  v8-devel
BuildRequires:  which

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
%setup -q -c -n zsdk-ptlib


%build
source /opt/rh/devtoolset-9/enable
%configure --enable-exceptions \
        --with-profiling=manual \
        --disable-pthread_kill \
        --disable-vxml \
        --disable-sdl \
        --disable-sasl \
        --disable-openldap \
        --disable-plugins \
        --enable-cpp14 \
        PTLIB_MAJOR=%{version_major} \
        PTLIB_MINOR=%{version_minor} \
        PTLIB_PATCH=%{version_patch} \
        PTLIB_OEM=%{version_oem}
make %{?_smp_mflags} REVISION_FILE= PTLIB_FILE_VERSION=%{version} all


%install
rm -rf $RPM_BUILD_ROOT
source /opt/rh/devtoolset-9/enable
make install DESTDIR=$RPM_BUILD_ROOT PTLIB_FILE_VERSION=%{version}
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
