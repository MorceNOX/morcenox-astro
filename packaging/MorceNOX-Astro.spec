# Recebe _version via --define no rpmbuild (usa 0.0.0 como fallback)
%global debug_package %{nil}
%{!?_version: %define _version 0.0.0}

Name:           MorceNOX-Astro
Version:        %{_version}
Release:        1%{?dist}
Summary:        Traditional Astrology calculation engine and TUI powered by Swiss Ephemeris

License:        GPL-3.0-or-later
URL:            https://github.com
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  gettext
BuildRequires:  ncurses-devel
BuildRequires:  libicu-devel

Requires:       ncurses-libs
Requires:       libicu

%description
MorceNOX-Astro is a terminal-based (ncurses) Traditional Astrology suite written
in C and backed by the Swiss Ephemeris calculation engine. It provides high-precision
astronomical and classical astrological computations following Hellenistic,
Arabic, and Renaissance methodologies.

Core capabilities include:
- Natal and horary/electional chart calculation with multiple house systems
- Classical Fixed Stars catalog (51 objects) with bodily conjunctions,
  declination parallels, local horizon/meridian Parans, and Schaefer-model
  Heliacal phases
- Zodiacal and mundane Primary Directions (with direct/converse arcs,
  parallels, and rapt parallels)
- Solar Return charts with Radix confrontation and fixed star activations
- Traditional Temperament analysis, Planetary Hours, Hyleg/Alcocoden
  determination, and Arabic Parts (Lots)
- Offline timezone resolution via ZoneDetect and embedded SQLite database


%prep
%autosetup

%build
%make_build PREFIX=/usr

%install
%make_install PREFIX=/usr

%files
%{_bindir}/astro
%{_datadir}/%{name}/
%{_datadir}/locale/*/LC_MESSAGES/*.mo
%{_datadir}/applications/morcenox-astro.desktop
%{_datadir}/icons/hicolor/256x256/apps/morcenox-astro.png

%changelog
* Tue Nov 17 2026 Amilcar Antonio Mesquita Rizk <amilcar.rizk@gmail.com>
- Automatized build from the Makefile

