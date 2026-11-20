# MorceNOX™ ASTRO
<img width="720" height="720" alt="MorceNOX-ASTRO-logo" src="MorceNOX-Astro.png" />

**Your advanced, terminal-based astrological companion.**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: Terminal](https://img.shields.io/badge/Platform-Terminal-lightgrey.svg)](https://en.wikipedia.org/wiki/Terminal)
[![GitHub Release](https://img.shields.io/github/v/release/MorceNOX/morcenox-astro?logo=github&label=Release)](https://github.com/MorceNOX/morcenox-astro/releases)
[![Copr build status](https://copr.fedorainfracloud.org/coprs/morcenox/morcenox-astro/package/MorceNOX-Astro/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/morcenox/morcenox-astro/package/MorceNOX-Astro/)

A Traditional Astrology application running in your terminal! Calculate your birth chart or quickly visualize the chart for the current moment. Check the planetary hours table to choose the best time for an activity, explore your mental and temperament analysis, navigate past and future events in a Primary Directions table, and much more!

**MorceNOX™ ASTRO** is a high-performance, visually striking, and accurate astrological engine designed for the terminal environment. Built in C with the Swiss Ephemeris, it provides astrologers with a powerful tool for calculating and visualizing complex classical charts—from Natal and Solar Return to advanced time-lord techniques like Firdaria and Primary Directions—alongside exact calculations for Pre-Natal Syzygy, Arabic Parts, and Fixed Stars.

Features a rich ASCII-art interface, real-motion celestial animation, and offline precision.

<hr>
<img width="2560" height="1600" alt="00-chart-wheel-v1 52 4" src="https://github.com/user-attachments/assets/db254bae-55d5-4eef-8d6e-b2e389bb62bf" />
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

##### Debian / Ubuntu / Linux Mint / Pop!_OS (`.deb`)
```bash
sudo apt update
sudo apt install ./morcenox-astro_X.Y.Z_amd64.deb
```

##### Arch Linux / Manjaro / EndeavourOS (`.pkg.tar.zst`)
```bash
sudo pacman -U ./morcenox-astro-X.Y.Z-1-x86_64.pkg.tar.zst
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
make -j\$(nproc)

# Install to the system (default prefix: /usr/local)
sudo make install
```

Then run the application simply by typing: `astro`


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

<img width="2560" height="1539" alt="01-main_menu_english" src="https://github.com/user-attachments/assets/a5f2df5f-e4a8-426d-af30-58573f4c8ad5" />

<hr>

### The Chart Wheel
<hr>

<img width="2560" height="1600" alt="00-chart-wheel-v1 52 4" src="https://github.com/user-attachments/assets/0e532721-9b87-4b2a-81fb-2de8eaac64e6" />

<img width="2560" height="1600" alt="02d-chart_wheel_zoomed" src="https://github.com/user-attachments/assets/09071b05-dc95-4f32-9dd3-dfe858b83a76" />

<img width="2560" height="1600" alt="02d-chart_wheel_zoomed2" src="https://github.com/user-attachments/assets/2821abc6-8e6f-4c0f-9779-a0735de5d42d" />

<hr>

### Aspects Table
<hr>

<img width="2560" height="1600" alt="03a-aspects_table" src="https://github.com/user-attachments/assets/ce3e0f8b-df08-4439-a161-ce2c3fef9456" />

<img width="2560" height="1600" alt="03c-parallel_aspects_table" src="https://github.com/user-attachments/assets/1b6d5b6b-895f-4648-816e-117da9167009" />

<img width="2560" height="1600" alt="03a-aspects_antissia" src="https://github.com/user-attachments/assets/7e5a1a9b-cbf7-421c-bb7f-998b25830ad3" />

<hr>

### Primary Directions
<hr>

<img width="2560" height="1600" alt="04b-primary_directions_table" src="https://github.com/user-attachments/assets/f861727f-9b56-4ae4-a627-a8ccb3a9aa53" />

<hr>

### Firdaria
<hr>

<img width="2560" height="1600" alt="12-firdaria" src="https://github.com/user-attachments/assets/0dfcd33a-9651-4f70-b7b5-e55c2bd5bd4a" />

<hr>

### Energy Profile Chart
<hr>

<img width="2560" height="1600" alt="07-energy_profile_RS" src="https://github.com/user-attachments/assets/3a572fab-9fb6-4920-b78c-bd8a71a6301d" />

<img width="2560" height="1600" alt="07a-energy_profile_chart" src="https://github.com/user-attachments/assets/ed1311d6-9301-48cc-9826-1c94c360d6bf" />

<hr>

### Temperament Chart
<hr>

<img width="2560" height="1600" alt="09a-temperament_chart" src="https://github.com/user-attachments/assets/02479101-34c8-4fb6-ab9f-70ea9e580a7d" />

<img width="2560" height="1600" alt="09a-temperament_chart" src="https://github.com/user-attachments/assets/0776e0fa-63e8-4359-b723-e2dae60ab2f4" />

<hr>

### Temperament Interpretation
<hr>

<img width="2560" height="1600" alt="09a-temperament_interpretation" src="https://github.com/user-attachments/assets/77cf7624-9b49-479c-8e13-5b5d76488193" />

<img width="2560" height="1600" alt="09a-temperament_interpretation" src="https://github.com/user-attachments/assets/94d814bf-1999-4f86-bbd5-98537bb2a7c4" />

<hr>

### Solar Return Chart
<hr>

<img width="2560" height="1600" alt="06-solar_return_chart" src="https://github.com/user-attachments/assets/e67bb184-fc01-498c-8165-6ea6f243c76a" />

<hr>

### Solar Return Radix Confrontation
<hr>

<img width="2560" height="1600" alt="06b-solar_return_radix_confrontation" src="https://github.com/user-attachments/assets/5a702582-b173-4d89-a39b-c4b51d8ca547" />
<hr>

### Solar Return Transit Projection
<hr>

<img width="2560" height="1600" alt="06a-solar_return_transit_projection" src="https://github.com/user-attachments/assets/ddd44ceb-3ab7-4b75-b2d7-ecfbe04493b4" />

<hr>

### Planetary Hours
<hr>

<img width="2560" height="1600" alt="10-planetary_hours" src="https://github.com/user-attachments/assets/56ffff4e-410d-45ed-aef2-cc02b5f4cb2e" />

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

