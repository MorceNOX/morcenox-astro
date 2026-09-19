# MorceNOX™ ASTRO
<img width="2390" height="2390" alt="logo-ascii-art" src="https://github.com/user-attachments/assets/b404be83-ea1b-4230-8f35-fa8e06f31fb6" />

**Your advanced, terminal-based astrological companion.**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: Terminal](https://img.shields.io/badge/Platform-Terminal-lightgrey.svg)](https://en.wikipedia.org/wiki/Terminal)

MorceNOX™ ASTRO is a high-performance,-precision astrological engine designed for the terminal environment. Built with C and the Swiss Ephemeris, it provides astrologers with a powerful tool for calculating and visualizing complex classical astrological charts, from Natal and Solar Revolution to advanced time-lord techniques like Firdaria and Primary Directions.

Features a stunning ASCII-art interface, real-motion animation, and deep advanced calculation engines.

## 🌌 Key Features

### 🛠 Advanced Charting & Calculations
* **Multi-Chart Support:** Generate Natal (Radix) charts, Solar Return (Annual) charts, and real-time Transit charts.
* **Solar Return Engine:** A sophisticated "Radix Confrontation" system that cross-examines the annual chart against the birth chart via seven distinct checkpoints (Almuten, House Transits, Time-Lord Co-Alignment, etc.). It also verifies the transits of the planets against the natal (radical) positions, including the Arabic Parts.
* **Time-Lord Techniques:** Built-in modules for **Firdaria**, **Annual Profections**, and **Vital Chronocrators** (Hyleg & Alcochoden).
* **Primary Directions:** Precise calculation of Direct and Converse directions for planets, angles and Arabic Parts using Naibod time key and dynamic obliquity for *zodiacal* and mundane proportional to semi-arc for directions *in-mundo*.
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
* Currently MorceNOX™ ASTRO is available in **English** and **Brazilian Portuguese** in the same version. You just need to select the language by the Settings menu.

## 🛠 Technical Stack

* **Core:** C
* **Ephemeris:** [Swiss Ephemeris](https://github.com/aloistr/swisseph) (for astronomical precision).
* **UI/UX:** `ncurses` (for the terminal-based interface).
* **Database:** [SQLite](https://github.com/sqlite/sqlite) (for storing charts, settings, and a massive city database).
* **Data Source:** Enhanced `countries-states-cities-database` (approx. 153k cities).
* **Internationalization:** `libicu` (Unicode/ICU support).

## 🚀 Installation & Requirements

*Note: As this is a C-based application, you will need a C compiler and the necessary development libraries installed on your system.*
`gcc` (for building from source)
`make` (for all running options)

### Prerequisites
Ensure you have the following libraries installed:

#### Swiss Ephemeris

Clone and build the Swiss Ephemeris repository (optional, but desirable):
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

## 🚀 Getting Started

There are two ways to use MorceNOX™ ASTRO.

### 🌟 Option 1: Portable Release (Recommended)
**No installation, no compiler, and no dependencies required.** Use this if you just want to run the app immediately.

1. Go to the [Releases](https://github.com/MorceNOX/morcenox-astro/releases) page.
2. Download the latest `MorceNOX-Astro-vX.Y.Z-linux.tar.gz`.
3. Extract it to your preferred folder:
   ```bash
   tar -xzvf MorceNOX-Astro-vX.Y.Z-linux.tar.gz
   ```
4. Run the launcher:
   ```
   cd MorceNOX-Astro-vX.Y.Z-linux
   ./astro.sh
   ```
The first time you run it, the application will automatically set up your configuration and database in $HOME/.config/MorceNOX-Astro.

### 🛠 Option 2: Building from Source

Use this if you want to customize the code or are working on a custom Linux distribution.

#### Prerequisites (Ubuntu/Debian):
```
sudo apt-get install build-essential libncurses-dev libicu-dev
```

#### Build Steps:
```
# Clone the repository
git clone https://github.com/MorceNOX/morcenox-astro.git
cd morcenox-astro

# Compile the engine
make -j$(nproc)

# Initialize your local user environment
make setup-dir

# Install to the system
sudo make install
```

Then run the application simply by typing: `astro`

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
    * `[←↓↑→]` - Pan.
    * `[R]` - Reset Zoom and Pan.    
* **Sub-Modules:** Accessed via Function and Numeric keys (e.g., `F1` for Data, `F3` for Aspects, `F12` for Firdaria, `3` for Primary Directions).

## Screenshots

Here are some screenshots of the application. You can have an idea of what you expect from this application. And there are many more modules to explore!

### Main Menu
<hr>
<img width="2560" height="1600" alt="01-main_menu" src="https://github.com/user-attachments/assets/c01bef01-0e2a-4b0e-8913-7583301d6eb8" />
<hr>

### The Chart Wheel
<hr>
<img width="2560" height="1600" alt="02a-chart_wheel_example" src="https://github.com/user-attachments/assets/53166ca7-7b90-4807-add9-30dbabf44d3d" />
---
<img width="2560" height="1600" alt="02a-chart_wheel_house_divisions" src="https://github.com/user-attachments/assets/8c70cddc-fb14-47dc-9b50-331634a35f31" />
---
<img width="2560" height="1600" alt="02b-chart_wheel_city_selection_rs" src="https://github.com/user-attachments/assets/0dee28f4-7e4b-4bed-bcb1-60e15b448dad" />
<hr>

### Aspects Tables
<img width="2560" height="1600" alt="03a-aspects_antissia" src="https://github.com/user-attachments/assets/0055e263-2b27-49fe-b9b6-f4930bf51bef" />
<img width="2560" height="1600" alt="03b-aspects_table" src="https://github.com/user-attachments/assets/f6b612ad-71ab-4549-a322-3d84061e6aa4" />
<img width="2560" height="1600" alt="03a-aspects_table" src="https://github.com/user-attachments/assets/c8928481-5b54-4ace-a2ad-d9064e4eb6cd" />
<hr>

### Primary Directions
<hr>
<img width="2560" height="1600" alt="04-primary_directions_table" src="https://github.com/user-attachments/assets/42048ac9-b94a-4a3f-9060-9fb6ecdc4c09" />
<hr>

### Firdaria
<hr>
<img width="2560" height="1600" alt="05a-firdaria_table" src="https://github.com/user-attachments/assets/9a85240a-d752-4ebf-b0c3-4487a4cc50f0" />
<hr>

### Energy Profile Chart
<hr>
<img width="2560" height="1600" alt="07-energy_profile_RS" src="https://github.com/user-attachments/assets/3a572fab-9fb6-4920-b78c-bd8a71a6301d" />
<img width="2560" height="1600" alt="07a-energy_profile_chart" src="https://github.com/user-attachments/assets/ed1311d6-9301-48cc-9826-1c94c360d6bf" />
<hr>

### Temperament Chart
<hr>
<img width="2560" height="1600" alt="09a-temperament_chart" src="https://github.com/user-attachments/assets/02479101-34c8-4fb6-ab9f-70ea9e580a7d" />
<hr>

### Temperament Interpretation
<hr>
<img width="2560" height="1600" alt="09a-temperament_interpretation" src="https://github.com/user-attachments/assets/77cf7624-9b49-479c-8e13-5b5d76488193" />
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

## 📜 License

This program is free software. You can redistribute and/or modify it under the terms of the **GNU General Public License (GPL)** as published by the Free Software Foundation.

## 👤 Author

**Amilcar Antonio Mesquita Rizk**
*Copyright © 2026*

---
*Disclaimer: This software is provided "as is", without warranty of any kind. Use for astrological research and professional practice at your own discretion.*

***

