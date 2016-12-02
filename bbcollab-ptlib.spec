%global version_major  2
%global version_minor  17
%global version_patch 1

Name:           bbcollab-ptlib
Version:        %{version_major}.%{version_minor}.%{version_patch}
Release:        1%{?jenkins_release}%{?dist}
Summary:        PTLib: Portable Tools Library

Group:          System Environment/Libraries
License:        MPL 1.0 + others
URL:            https://stash.bbpd.io/projects/COL/repos/zsdk-ptlib/browse
Source0:        zsdk-ptlib.src.tgz

BuildRequires:  %__sed
BuildRequires:  bbcollab-gcc >= 5.1.0
BuildRequires:  bbcollab-openssl-devel
BuildRequires:  gperftools
# The following are recommended here: http://wiki.opalvoip.org/index.php?n=Main.BuildingPTLibUnix
BuildRequires:  cyrus-sasl-devel
BuildRequires:  expat-devel
# gstreamer 0.10 support seems to be broken, but 1.0 isn't available
#BuildRequires:  gstreamer-plugins-base-devel
#BuildRequires:  gstreamer-devel
BuildRequires:  libjpeg-devel
BuildRequires:  libpcap-devel
BuildRequires:  lua-devel
BuildRequires:  ncurses-devel
BuildRequires:  openldap-devel
BuildRequires:  SDL-devel
BuildRequires:  unixODBC-devel
BuildRequires:  v8-devel

%description
PTLib: Portable Tools Library

%package        devel
Summary:        Development files for %{name}
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}
Requires:       bbcollab-openssl-devel

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
PKG_CONFIG_PATH=/opt/bbcollab/lib64/pkgconfig:/opt/bbcollab/lib/pkgconfig
%configure --prefix=/opt/bbcollab \
        --exec-prefix=/opt/bbcollab \
        --bindir=/opt/bbcollab/bin \
        --sbindir=/opt/bbcollab/sbin \
        --sysconfdir=/opt/bbcollab/etc \
        --datadir=/opt/bbcollab/share \
        --includedir=/opt/bbcollab/include \
        --libdir=/opt/bbcollab/lib64 \
        --libexecdir=/opt/bbcollab/libexec \
        --localstatedir=/opt/bbcollab/var \
        --sharedstatedir=/opt/bbcollab/var/lib \
        --mandir=/opt/bbcollab/share/man \
        --infodir=/opt/bbcollab/share/info \
        --enable-exceptions \
        --with-profiling=manual \
        --disable-pthread_kill \
        --disable-vxml \
        --disable-sdl \
        --disable-sasl \
        --disable-openldap \
        --disable-plugins \
        --enable-cpp14 \
        OPT_CFLAGS=-fno-strict-aliasing \
        CC=/opt/bbcollab/bin/gcc \
        CXX=/opt/bbcollab/bin/g++ \
        LD=/opt/bbcollab/bin/g++ \
        PTLIB_MAJOR=%{version_major} \
        PTLIB_MINOR=%{version_minor} \
        PTLIB_BUILD=%{version_patch}
make %{?_smp_mflags} all


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
find $RPM_BUILD_ROOT -name '*.la' -exec rm -f {} ';'


%clean
rm -rf $RPM_BUILD_ROOT


# No need to call ldconfig, as we're not installing to the default dynamic linker path
#%post -p /sbin/ldconfig

#%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%doc ReadMe.txt History.txt
/opt/bbcollab/lib64/*.so.*

%files devel
%defattr(-,root,root,-)
/opt/bbcollab/include/*
/opt/bbcollab/lib64/*.so
/opt/bbcollab/lib64/pkgconfig/*.pc
/opt/bbcollab/share/ptlib/make/*

%files static
%defattr(-,root,root,-)
/opt/bbcollab/lib64/*.a


%changelog
* Mon Nov 21 2016 Gavin Llewellyn <gavin.llewellyn@blackboard.com> - 2.17.1-1
- Initial RPM release
