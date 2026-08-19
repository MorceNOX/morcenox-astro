# MorceNOX™ ASTRO
<img width="1344" height="768" alt="MorceNOX-ASTRO-logo" src="https://github.com/user-attachments/assets/5ccde602-e15f-4001-8956-b322583fd432" />

**Your advanced, terminal-based astrological companion.**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: Terminal](https://img.shields.io/badge/Platform-Terminal-lightgrey.svg)](https://en.wikipedia.org/wiki/Terminal)

MorceNOX™ ASTRO is a high-performance,-precision astrological engine designed for the terminal environment. Built with C and the Swiss Ephemeris, it provides astrologers with a powerful tool for calculating and visualizing complex classical astrological charts, from Natal and Solar Revolution to advanced time-lord techniques like Firdaria and Primary Directions.

Features a stunning ASCII-art interface, real-motion animation, and deep mathematical confrontation modules.

## 🌌 Key Features

### 🛠 Advanced Charting & Calculations
* **Multi-Chart Support:** Generate Natal (Radix) charts, Solar Return (Annual) charts, and real-time Transit charts.
* **Solar Return Engine:** A sophisticated "Radix Confrontation" system that cross-examines the annual chart against the birth chart via seven distinct checkpoints (Almuten, House Transits, Time-Lord Co-Alignment, etc.).
* **Time-Lord Techniques:** Built-in modules for **Firdaria**, **Annual Profections**, and **Vital Chronocrators** (Hyleg & Alcochoden).
* **Primary Directions:** Precise calculation of Direct and Converse directions for planets and Arabic Parts using Naibod time key and dynamic obliquity.
* **Arabic Parts:** Create, edit, and manage a custom collection of Arabic Parts, with automated aspect calculation.
* **Anareta & Vital Threats:** Identify potential physical risks and vital threats based on the 8th house ruler and the Anareting planet.

### 📐 Mathematical Precision
* **Astrological Depth:** Includes Almuten Figuris/Hylegiacal analysis, planetary strength scoring (Essential vs. Accidental dignities), and Temperament/Mind analysis.
* **House Systems:** Supports a wide array of systems including Whole Signs, Campanus, Regiomontanus, Placidus, Koch, and Topocentric.
* **Planetary Hours:** Full calculation of planetary hours for day and night, synchronized with sunrise/sunset.

### 🖥 Interactive Terminal Interface
* **Visual Excellence:** High-resolution ASCII art chart wheels with color-coded elements (Fire, Earth, Air, Water) and planetary types.
* **Dynamic Interaction:** Zoom in/out, pan the chart, and toggle overlays like Decans, Terms (Bounds), and House Boundaries.
* **Animation Mode:** Visualize the past and future movement of celestial bodies by adjusting the animation pace.
* **Full Offline Capability:** 100% offline operation. No data is sent to or retrieved from the internet.

### 🌎 Multilingual Support
* Currently only **English** and **Brazilian Portuguese** are available.

## 🛠 Technical Stack

* **Core:** C
* **Ephemeris:** [Swiss Ephemeris](https://github.com/aloistr/swisseph) (for astronomical precision).
* **UI/UX:** `ncurses` (for the terminal-based interface).
* **Database:** `SQLite` (for storing charts, settings, and a massive city database).
* **Data Source:** Enhanced `countries-states-cities-database` (approx. 153k cities).
* **Internationalization:** `libicu` (Unicode/ICU support).

## 🚀 Installation & Requirements

*Note: As this is a C-based application, you will need a C compiler and the necessary development libraries installed on your system.*
`gcc`
`make`

### Prerequisites
Ensure you have the following libraries installed:

#### Swiss Ephemeris

Clone and build the Swiss Ephemeris repository:
```
git clone https://github.com/aloistr/swisseph.git

cd swisseph
make -j$(nproc)

# copy the ephemeris files to your home directory
cp -r ephe ~/ephe

# create the environment variable
echo "export SE_EPHE_PATH="$HOME"/ephe >> .bashrc
```

#### For Ubuntu/Debian based systems
* `libncurses-dev`
* `libicu-dev`

```
sudo apt-get install build-essentials gcc make libncurses-dev libicu-dev
```

#### For Fedora/Red-Hat/CentOS
* `ncurses-devel`
* `libicu-devel`
```
sudo dnf install gcc make ncurses-devel libicu-devel
```

Clone and Build the SQLite database with libicu
```
git clone https://github.com/sqlite/sqlite.git
cd sqlite

export CFLAGS="-DSQLITE_ENABLE_ICU -licui18n -licuuc -licudata"
../configure --enable-fts5 --with-icu-config --enable-icu-collations
make -j$(nproc)
sudo make install
```


### Building from Source
```bash
# Clone the repository
git clone https://github.com/MorceNOX/morcenox-astro.git
cd morcenox-astro

# Compile
make -j$(nproc)

# Create User Environment
make setup-dir

# Install
sudo make install
```

## ⌨️ Usage & Navigation

MorceNOX™ Astro is designed for speed. Most actions require only one or two keystrokes.

* **Main Menu:** Use `Arrow Keys` to navigate and `Enter` to select.
* **Chart Window:**
    * `[M]` - Open Action Menu.
    * `[A]` - Toggle Animation.
    * `[+]` / `[-]` - Zoom In / Zoom Out.
    * `[D]` - Display Decans.
    * `[B]` - Display Terms (Bounds).
    * `[H]` - Display House Boundaries.
    * `[R]` - Reset Zoom and Pan.
* **Sub-Modules:** Accessed via Function keys (e.g., `F1` for Data, `F3` for Aspects, `F12` for Firdaria).

## Screenshots

Here are some screenshots of the application. You can have an idea of what you expect from this application. And there are many more mudules to explore!

### Main Menu

#### In Brazilian Portuguese
<hr>
<img width="2560" height="1600" alt="01-main_menu" src="https://github.com/user-attachments/assets/5f0b01f9-e766-42c8-a758-53b6c88ec0c4" />

#### In English
<hr>
<img width="2560" height="1600" alt="01-main_menu_english" src="https://github.com/user-attachments/assets/1d610c6e-5e09-49bd-bf51-bfce54658ede" />
<hr>

### The Chart Wheel
<hr>
<img width="2560" height="1600" alt="02-chart_wheel_example_02" src="https://github.com/user-attachments/assets/ab0f7407-1705-4182-97a1-9f9a18659ba6" />
<hr>

### The Chart Wheel Zoomed
<hr>
<img width="2560" height="1600" alt="02-chart_wheel_zoomed_02" src="https://github.com/user-attachments/assets/d56feca5-1321-4ca9-a36a-560ba532a332" />
<hr>

### Dignities and Rulership Table 1
<hr>
<img width="2560" height="1600" alt="03-dignities_rulership_table" src="https://github.com/user-attachments/assets/d475b506-b772-4fa2-a553-03fb5e3018d2" />
<hr>

### Aspects Table
---
<img width="2560" height="1600" alt="03a-aspects_table" src="https://github.com/user-attachments/assets/a4bfc3a6-cba2-4e68-ad21-367437019345" />
<hr>

### Primary Directions
<hr>
<img width="2560" height="1600" alt="04-primary_directions_table" src="https://github.com/user-attachments/assets/88620cca-fb86-4e6f-8a83-60a2ee5c0e82" />
<hr>

### Firdaria
<hr>
<img width="2560" height="1600" alt="05-firdaria_table" src="https://github.com/user-attachments/assets/90fb87e2-a310-412e-87b5-bcf531d7192b" />
<hr>

### Energy Profile Chart
<hr>
<img width="2560" height="1600" alt="07-energy_profile_chart" src="https://github.com/user-attachments/assets/4e7de0d3-2d46-44db-bad0-c456793ed5c9" />
<hr>

## 📜 License

This program is free software. You can redistribute and/or modify it under the terms of the **GNU General Public License (GPL)** as published by the Free Software Foundation.

## 👤 Author

**Amilcar Antonio Mesquita Rizk**
*Copyright © 2026*

---
*Disclaimer: This software is provided "as is", without warranty of any kind. Use for astrological research and professional practice at your own discretion.*

***

