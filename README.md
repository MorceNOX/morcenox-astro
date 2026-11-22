# MorceNOX™ ASTRO

<p align="center">
    <img height="509" alt="MorceNOX-ASTRO-logo" src="MorceNOX-Astro.png" />
</p>

<p align="right">
  <a href="README.md"><img src="https://flagcdn.com/w40/gb.png" width="25" alt="English"></a>
  <a href="README-pt.md"><img src="https://flagcdn.com/w40/br.png" width="25" alt="Português"></a>
</p>

**Your advanced, terminal-based astrological companion.**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: Terminal](https://img.shields.io/badge/Platform-Terminal-lightgrey.svg)](https://en.wikipedia.org/wiki/Terminal)
[![GitHub Release](https://img.shields.io/github/v/release/MorceNOX/morcenox-astro?logo=github&label=Release)](https://github.com/MorceNOX/morcenox-astro/releases)
[![Build & Publish Universal Linux Packages](https://github.com/MorceNOX/morcenox-astro/actions/workflows/release.yml/badge.svg)](https://github.com/MorceNOX/morcenox-astro/actions/workflows/release.yml)
[![Copr build status](https://copr.fedorainfracloud.org/coprs/morcenox/morcenox-astro/package/MorceNOX-Astro/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/morcenox/morcenox-astro/package/MorceNOX-Astro/)

A Traditional Astrology application running in your terminal! Calculate your birth chart or quickly visualize the chart for the current moment. Check the planetary hours table to choose the best time for an activity, explore your mental and temperament analysis, navigate past and future events in a Primary Directions table, and much more!

**MorceNOX™ ASTRO** is a high-performance, visually striking, and accurate astrological engine designed for the terminal environment. Built in C with the Swiss Ephemeris, it provides astrologers with a powerful tool for calculating and visualizing complex classical charts—from Natal and Solar Return to advanced time-lord techniques like Firdaria and Primary Directions—alongside exact calculations for Pre-Natal Syzygy, Arabic Parts, and Fixed Stars.

Features a rich ASCII-art interface, real-motion celestial animation, and offline precision.

<hr>

![01-chart-wheel-pres-en](https://github.com/user-attachments/assets/7cba1e55-06ab-4388-897a-650f84a2cd2b)

<hr>


## 🌌 Key Features

### 🛠 Advanced Charting & Calculations
* **Multi-Chart Support:** Generate Natal (Radix) charts, Solar Return (Annual) charts, and real-time  charts with animation.
* **Temperament Analysis** An accurate calculation of the native's Temperament, displaying a chart with the predominant temperaments and interpretation.
* **Strength of the Planets:** Calculate the **Almuten Figuris** & the **Strength of the Planets**, according to *Essential* and *Accidental Dignities*. 
* **Solar Return Engine:** A sophisticated "Radix Confrontation" system that cross-examines the annual chart against the birth chart via seven distinct checkpoints (Almuten, House Transits, Time-Lord Co-Alignment, Ascendant, etc.). It also verifies the transits of the planets against the natal (radical) positions, including the Arabic Parts & the **Activation of the Fixed Stars**, as well the stars positions/status in the radical chart.
* **Time-Lord Techniques:** Built-in modules for **Firdaria**, **Annual Profections**, and **Vital Chronocrators** (Hyleg & Alcochoden).
* **Primary Directions:** Precise calculation of Direct and Converse directions for planets, angles and Arabic Parts using several time keys and three methods for both, *zodiacal with latitude* and *mundane*, including directions to Arabic Parts and directing significators to Fixed Stars.
* **Arabic Parts:** Create, edit, and manage a custom collection of Arabic Parts, with automated aspect calculation.
* **Anareta & Vital Threats:** Identify potential physical risks and vital threats based on the 8th house ruler and the Anareting planet.
* **Fixed Stars**: Select, calculate, see their **Aspects**, **Parans**, **Heliacal Phase** and the **Activation** of the **Fixed Stars** in the Solar Return Chart, and the option to include them as promissors in the Primary Directions;
* and more!

### 📐 Mathematical Precision
* **Astrological Depth:** Includes Almuten Figuris/Hylegiacal analysis, planetary strength scoring (Essential vs. Accidental dignities), and Temperament/Mind analysis.
* **House Systems:** Supports a wide array of systems including Whole Signs, Campanus, Regiomontanus, Placidus, Koch, and Topocentric.
* **Planetary Hours:** Full calculation of planetary hours for day and night, synchronized with sunrise/sunset.

### 🖥 Interactive Terminal Interface
* **Visual Excellence:** High-resolution ASCII art chart wheels with vibrant colors and color-coded elements (Fire, Earth, Air, Water) and planetary types.
* **Dynamic Interaction:** Zoom in/out, pan the chart, and toggle overlays like Decans, Terms (Bounds), and House Boundaries.
* **Animation Mode:** Visualize & control the past and future movement of celestial bodies by adjusting the animation pace.
* **Full Offline Capability:** 100% offline operation. No data is sent to or retrieved from the internet.
* **Mouse Support:** Double click an option of the menus to execute it! Chart Wheel and all windows are clickable. You can pan the chart wheel by dragging and dropping. Double clicking makes the chart zoomed in and out; Every button and scrollbar are clickable, as well as the windows.
* **Extensive Help:** You can read in a proper window about any part of the program just by selecting a topic.

### 🌎 Multilingual Support
* MorceNOX™ ASTRO is available entirely in **English** and **Brazilian Portuguese** in the same version. You just need to select the language by the Settings menu.

## 🛠 Technical Stack

* **Core:** C
* **Ephemeris:** [Swiss Ephemeris](https://github.com/aloistr/swisseph) (for astronomical precision).
* **UI/UX:** `ncurses` (for the terminal-based interface).
* **Database:** [SQLite](https://github.com/sqlite/sqlite) (for storing charts, settings, and a massive city database).
* **Data Source:** Enhanced `countries-states-cities-database` (approx. 153k cities).
* **Internationalization:** `libicu` (Unicode/ICU support) and `ZoneDetect` (https://github.com/BertoldVdb/ZoneDetect).

## 🚀 Getting Started

There are three ways to install and run **MorceNOX™ ASTRO** on Linux.

### 🐧 Option 1: Native Linux Packages (Recommended)

#### Fedora / RHEL / CentOS — Via Fedora COPR (Automatic Updates)
For Fedora users, we recommend enabling our official [Fedora COPR repository](https://copr.fedorainfracloud.org/coprs/morcenox/morcenox-astro/) (`x86_64` and `aarch64`) and installing the program directly via `dnf` to receive automatic updates:

```bash
sudo dnf copr enable morcenox/morcenox-astro
sudo dnf install MorceNOX-Astro
```

---

#### Manual Package Installation (`.rpm`, `.deb`, `.pkg.tar.zst`)
Alternatively, pre-built packages are available for the major Linux distribution families. Both **Swiss Ephemeris** and **SQLite3** (with ICU support) are statically embedded in the binary, and all required runtime libraries (`ncurses` and `libicu`) are resolved automatically by your package manager.

1. Go to the [Releases](https://github.com/MorceNOX/morcenox-astro/releases) page and download the package for your distribution.
2. Install using your system's package manager:

##### Fedora / RHEL / CentOS (`.rpm`)
```bash
sudo dnf install ./MorceNOX-Astro-X.Y.Z-1.*.x86_64.rpm
```
or
```bash
sudo dnf install ./MorceNOX-Astro-X.Y.Z-1.*.aarch64.rpm
```

##### Debian / Ubuntu / Linux Mint / Pop!_OS (`.deb`)
```bash
sudo apt update
sudo apt install ./morcenox-astro_X.Y.Z_amd64.deb
```
or
```bash
sudo apt update
sudo apt install ./morcenox-astro_X.Y.Z_arm64.deb
```

##### Arch Linux / Manjaro / EndeavourOS (`.pkg.tar.zst`)
```bash
sudo pacman -U ./morcenox-astro-X.Y.Z-1-x86_64.pkg.tar.zst
```
or
```bash
sudo pacman -U ./morcenox-astro-X.Y.Z-1-aarch64.pkg.tar.xz
```

3. Launch the application from any terminal:
```bash
astro
```
> **Note:** On the very first run, `astro` automatically creates `$HOME/.config/MorceNOX-Astro`, sets up your configuration files, and initializes the SQLite city database.

---

### 🌟 Option 2: Portable Release

Use this if you prefer to run the application from a self-contained folder without system-wide installation (requires `ncurses` and `libicu` runtime libraries on the host).

1. Go to the [Releases](https://github.com/MorceNOX/morcenox-astro/releases) page.
2. Download the latest `MorceNOX-Astro-vX.Y.Z-linux.tar.gz`.
3. Extract it to your preferred folder:
   ```bash
   tar -xzvf MorceNOX-Astro-vX.Y.Z-linux.tar.gz
   ```
4. Run the launcher:
   ```bash
   cd MorceNOX-Astro-vX.Y.Z-linux
   ./astro.sh
   ```

---

### 🛠 Option 3: Building from Source

Use this if you want to customize the code or build for a custom Linux setup.

#### Prerequisites:
* **Fedora / RHEL:**
  ```bash
  sudo dnf install gcc make gettext ncurses-devel libicu-devel
  ```
* **Ubuntu / Debian:**
  ```bash
  sudo apt-get install build-essential gettext libncurses-dev libicu-dev
  ```
* **Arch Linux:**
  ```bash
  sudo pacman -S base-devel gettext ncurses icu
  ```

#### Build & Install Steps:
```bash
# Clone the repository
git clone https://github.com/MorceNOX/morcenox-astro.git
cd morcenox-astro

# Compile the engine and translations
make -j $(nproc)

# Install to the system (default prefix: /usr/local)
sudo make install
```

Then run the application simply by typing: `astro`

---

### 🖥️ Desktop Integration

Although **MorceNOX™ ASTRO** is a terminal-based (`ncurses`) application, it includes full Freedesktop (`.desktop`) and high-resolution icon integration:

* **System Packages (`.rpm`, `.deb`, Arch):** Automatically installs the application shortcut and icon into `/usr/share/applications` and `/usr/share/icons`, making **MorceNOX Astro** immediately searchable from your desktop environment's application menu (GNOME, KDE, XFCE, etc.).
* **Portable Release (`.tar.gz`):** On the first run of `./astro.sh`, the launcher automatically registers a local desktop entry and icon inside `~/.local/share/applications` pointing to your extracted folder—no `root` privileges required.


## ⌨️ Usage & Navigation

MorceNOX™ Astro is designed for speed, calculation accuracy, and to be a beautiful and an easy to use astrological application. Most actions require only one or two keystrokes, or a click or double-click.

* **Main Menu:** Use `Arrow Keys` to navigate and `Enter` to select, or `double click` an option.
* **Chart Window:**
    * `[M]` - Open Action Menu.
    * `[A]` - Toggle Animation.
    * `[+]` / `[-]` - Zoom In / Zoom Out.
    * `[D]` - Display Decans.
    * `[B]` - Display Terms (Bounds).
    * `[H]` - Display House Boundaries.
    * `[←↓↑→] or drag and drop` - Pan.
    * `[R]` - Reset Zoom and Pan.
* **Sub-Modules:** Accessed via Function and Numeric keys (e.g., `F1` for Data, `F3` for Aspects, `F12` for Firdaria, `3` for Primary Directions), and many more. Select the `Help` option to read about any part of the application.

## Screenshots

Here are some screenshots of the application. You can have an idea of what you expect from this application. And there are many more modules to explore!

### Main Menu
<hr>

![00-main-menu-en](https://github.com/user-attachments/assets/5aec2f79-b6a9-4e13-8196-1aba264a4ebd)

<hr>

### The Chart Wheel
<hr>

![01-chart-wheel-en](https://github.com/user-attachments/assets/972de35e-63a7-4a76-953f-8f15d6fa0f00)

![01-chart-zoom-en](https://github.com/user-attachments/assets/e16ab238-4b77-40a4-bae7-d204d172cda9)

![01-chart-zoom-house-div-en](https://github.com/user-attachments/assets/8cf21e65-39a1-4cde-8749-985e0d73aa05)

<hr>

### Aspects Table
<hr>

![10-aspects-en](https://github.com/user-attachments/assets/f57e087c-a478-49bb-ba23-9712ed9422c4)

![10-aspects-parallel-en](https://github.com/user-attachments/assets/1684cdbf-fce3-401c-91fb-995b1f44d8dd)

![09-houses-aspects-en](https://github.com/user-attachments/assets/29b042f0-d563-460f-812f-e9acafe751b8)

<hr>

### Primary Directions
<hr>

![02-directions-en](https://github.com/user-attachments/assets/8873b918-6861-4b89-930f-bb95382b958e)

<hr>

### Firdaria
<hr>

![04-firdaria-en](https://github.com/user-attachments/assets/e91b3015-3f8d-494c-be7b-b3732c962cf1)

<hr>

### Energy Profile Chart
<hr>

![06-energy-en](https://github.com/user-attachments/assets/0e293615-7b0a-4ac0-97d5-32678fc7cb08)

<hr>

### Temperament Chart
<hr>

![08-temperament-en](https://github.com/user-attachments/assets/f7a9f5ae-0aa0-435e-a06a-1a2a1ba53234)

<hr>

### Temperament Interpretation
<hr>

![08-temperament-interpretation-en](https://github.com/user-attachments/assets/cc22feec-5ebb-487d-b279-703bcf4a38e3)

<hr>

### Solar Return Chart
<hr>

![03-solar-return-chart-en](https://github.com/user-attachments/assets/32f08cc4-c36c-4943-8c09-2bc3bed9d365)

<hr>

### Solar Return Radix Confrontation
<hr>

![03-solar-return-confront-en](https://github.com/user-attachments/assets/9c43b115-d1f2-436b-a4ef-6fabf32106f7)

<hr>

### Solar Return Transit Projection
<hr>

![03-solar-return-transit-en](https://github.com/user-attachments/assets/9259b159-2e0f-4b46-951d-e1fd6f6f4230)

<hr>

### Planetary Hours
<hr>

![07-planetary-hours-en](https://github.com/user-attachments/assets/022414d3-15bd-4ed3-891d-56acb215c72e)

<hr>

## Licenses


### 📜 Licenses of 3rd Party Libraries and Resources




#### • Swiss Ephemeris

  https://github.com/aloistr/swisseph

  GNU Affero General Public License (AGPL)

  <https://www.gnu.org/licenses/agpl-3.0.ca.html>




#### • countries-states-cities-database

  https://github.com/dr5hn/countries-states-cities-database

  ODbL-1.0 license

  <https://opendatacommons.org/licenses/odbl/1.0/>




#### • SQLite

  https://sqlite.org/

  Public Domain




#### • ncurses

  https://invisible-island.net/ncurses/

  NCURSES License

  <https://invisible-island.net/ncurses/ncurses-license.html>




#### • ICU library (libicu)

  https://icu.unicode.org/

  (https://github.com/unicode-org/icu/blob/main/LICENSE)




#### • ZoneDetect

  https://github.com/BertoldVdb/ZoneDetect

  <https://github.com/BertoldVdb/ZoneDetect#BSD-3-Clause-1-ov-file>

  Copyright (c) 2018-2019, Bertold Van den Bergh

  (vandenbergh@bertold.org) All rights reserved.

<hr>



### License

This program is free software. You can redistribute and/or modify it under the terms of the **GNU General Public License (GPL)** as published by the Free Software Foundation.

> All of this is being done to put into practice what I have learned about astrology and the associated calculations. Therefore, it is possible that someone might find something incorrect or unusual. In that case, I would appreciate any help or suggestions to make this application even better.

## 👤 Author

**Amilcar Antonio Mesquita Rizk**
*Copyright © 2026*

---
*Disclaimer: This software is provided "as is", without warranty of any kind. Use for astrological research and professional practice at your own discretion.*

***

