Name:           adaptmon
Source0:        %{name}.tar.gz
Source1:        %{name}.git-version
%if 0%{?builddep_only}
%global version_major 0
%global version_minor 0
%global version_patch 0
%global git_version builddep
%else
%global version_major %(tar -xOf %{SOURCE0} %{name}/CMakeLists.txt | grep -w -m1 VERSION_MAJOR | cut -d' ' -f2 | tr -cd 0-9)
%global version_minor %(tar -xOf %{SOURCE0} %{name}/CMakeLists.txt | grep -w -m1 VERSION_MINOR | cut -d' ' -f2 | tr -cd 0-9)
%global version_patch %(tar -xOf %{SOURCE0} %{name}/CMakeLists.txt | grep -w -m1 VERSION_PATCH | cut -d' ' -f2 | tr -cd 0-9)
%global git_version %(cat %{SOURCE1})
%endif
Version:        %{version_major}.%{version_minor}.%{version_patch}.%{git_version}
Release:        1%{?dist}
Summary:        Adaptive network monitoring libraries and tools

License:        BSD-3-Clause
URL:            https://gitlab.liberouter.org/monitoring/adaptmon

BuildRequires:  cmake >= 3.25
BuildRequires:  gcc-c++
BuildRequires:  git-core
BuildRequires:  make
BuildRequires:  pkgconfig(libdpdk) >= 21.11
BuildRequires:  pkgconfig(libpcap)
%if 0%{?rhel} == 8 || 0%{?rhel} == 9
%global gcc_toolset 14
BuildRequires:  gcc-toolset-14-gcc-c++
%endif
%if 0%{?rhel} >= 10
%global gcc_toolset 15
BuildRequires:  gcc-toolset-15-gcc-c++
BuildRequires:  gcc-toolset-15-gcc-plugin-annobin
%endif

# disables generation of the shared "adaptmon-debuginfo" package to make it clearer which -debuginfo package is needed
%global _find_debuginfo_dwz_opts %{nil}

%description
Adaptive network monitoring libraries and command-line tools.

%package -n libamon
Summary:        Core AdaptMon library

%description -n libamon
Core packet processing and utility library used by AdaptMon applications.

%package -n libamon-dpdk
Summary:        DPDK support for libamon
Requires:       libamon%{?_isa} = %{version}-%{release}

%description -n libamon-dpdk
DPDK integration library for libamon.

%package -n amon-tools
Summary:        AdaptMon command-line tools
Requires:       libamon%{?_isa} = %{version}-%{release}

%description -n amon-tools
Command-line tools based on libamon.

%prep
%autosetup -n %{name}

%build
%if 0%{?rhel} == 8 || 0%{?rhel} == 9
source /opt/rh/gcc-toolset-%{gcc_toolset}/enable
%endif
%if 0%{?rhel} >= 10
eval "$(gcc-toolset-%{gcc_toolset}-env printenv | grep gcc-toolset)"
%endif
%cmake \
    -DBUILD_SHARED_LIBS=ON \
    -DENABLE_DPDK=ON \
    -DENABLE_APPS=ON \
    -DENABLE_PCAP=ON \
    -DENABLE_EXAMPLES=OFF \
    -DENABLE_TESTS=OFF
%cmake_build

%install
%cmake_install

%files -n libamon
%doc README.md lib/README.md
%{_libdir}/libamon.so.0
%{_libdir}/libamon.so.0.*
%{_libdir}/libamon.so
%{_includedir}/amon/
%exclude %{_includedir}/amon/dpdk/

%files -n libamon-dpdk
%{_libdir}/libamon-dpdk.so.0
%{_libdir}/libamon-dpdk.so.0.*
%{_libdir}/libamon-dpdk.so
%{_includedir}/amon/dpdk/

%files -n amon-tools
%{_bindir}/amon-anonymizer
%{_bindir}/amon-stats
%{_bindir}/amon-receiver

%changelog
* Tue Aug 25 2026 Michal Sedlak <sedlakm@cesnet.cz> - 0.1.0-1
- Initial package
