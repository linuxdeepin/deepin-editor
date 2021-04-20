%global debug_package   %{nil}
%define pkgrelease  1
%if 0%{?openeuler}
%define specrelease %{pkgrelease}
%else
## allow specrelease to have configurable %%{?dist} tag in other distribution
%define specrelease %{pkgrelease}%{?dist}
%endif

Name:           deepin-editor
Version:        5.9.2.1
Release:        %{specrelease}
Summary:        Simple editor for Linux Deepin
License:        GPLv3
URL:            https://github.com/linuxdeepin/deepin-editor
Source0:        %{url}/archive/%{version}/%{name}-%{version}.tar.gz

BuildRequires: cmake3
BuildRequires: qt5-devel
BuildRequires: gcc-c++
BuildRequires: freeimage-devel
BuildRequires: pkgconfig(dtkwidget) >= 2.0.6
BuildRequires: pkgconfig(dtkwm)
BuildRequires: pkgconfig(libexif)
BuildRequires: pkgconfig(xcb-aux)
BuildRequires: pkgconfig(xtst)
BuildRequires: pkgconfig(polkit-qt5-1)
# BuildRequires: pkgconfig(Qt5)
# BuildRequires: pkgconfig(Qt5DBus)
# BuildRequires: pkgconfig(Qt5Gui)
# BuildRequires: pkgconfig(Qt5Svg)
# BuildRequires: pkgconfig(Qt5X11Extras)
BuildRequires: pkgconfig(dframeworkdbus)
# BuildRequires: qt5-linguist
# BuildRequires: qt5-qtbase-private-devel
BuildRequires: cmake(KF5Codecs)
BuildRequires: cmake(KF5SyntaxHighlighting)
BuildRequires: gtest-devel
BuildRequires: gmock-devel



%description
%{summary}.

%prep
%setup -q

%build
# help find (and prefer) qt5 utilities, e.g. qmake, lrelease
export PATH=%{_qt5_bindir}:$PATH
sed -i "s|^cmake_minimum_required.*|cmake_minimum_required(VERSION 3.0)|" $(find . -name "CMakeLists.txt")
%cmake . -DVERSION=%{version}
%make_build

%install
%make_install

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/%{name}.desktop ||:

%files
%doc README.md
%license LICENSE
%{_bindir}/dedit
%{_bindir}/%{name}
%{_datadir}/%{name}/
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/scalable/apps/%{name}.svg
%{_datadir}/deepin-manual/manual-assets/application/deepin-editor/editor/*

%changelog
* Tue Apr 20 2021 zhangdingwen <zhangdingwen@uniontech.com> - 5.9.2.1-1
- init spec for euler
