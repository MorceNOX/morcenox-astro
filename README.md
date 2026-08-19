# MorceNOX™ ASTRO

[!Logo](MorceNOX-ASTRO-logo.png)

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

## 🛠 Technical Stack

* **Core:** C
* **Ephemeris:** [Swiss Ephemeris](https://github.com/aloistr/swisseph) (for astronomical precision).
* **UI/UX:** `ncurses` (for the terminal-based interface).
* **Database:** `SQLite` (for storing charts, settings, and a massive city database).
* **Data Source:** Enhanced `countries-states-cities-database` (approx. 153k cities).
* **Internationalization:** `libicu` (Unicode/ICU support).

## 🚀 Installation & Requirements

*Note: As this is a C-based application, you will need a C compiler and the necessary development libraries installed on your system.*

### Prerequisites
Ensure you have the following libraries installed:
* `libncurses-dev`
* `libswisseph-dev`
* `libsqlite3-dev`
* `libicu-dev`

### Building from Source
```bash
# Clone the repository
git clone https://github.com/MorceNOX/morcenox-astro.git
cd morcenox-astro

# Compile
make $(nproc)

# Create User Environment
make setup-dir

# Install
make install
```

## ⌨️ Usage & Navigation

MorceNOX™ is designed for speed. Most actions require only one or two keystrokes.

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
***
### Main Menu
***
[!Main Menu Portuguese](screenshots/01-main_menu.png)
***

### The Chart Wheel
***
[!Main Menu English](screenshots/01-chart_wheel_example_02.png)
***

### The Chart Wheel
***
[!Chart Wheel](screenshots/02-main_menu.png)
***

### Dignities and Rulership Table 1
***
[!Dignities](screenshots/03-dignities_rulership_table.png)
***

### Aspects Table
***
[!Aspects](screenshots/03a-aspects_table.png)
***

### Primary Directions
***
[!Directions](screenshots/04-primary_directions_table.png)
***

### Firdaria
***
[!Firdaria](screenshots/04-firdaria_table.png)
***

### Energy Profile Chart
***
[!Energy](screenshots/07-energy_profile_chart.png)
***

## 📜 License

This program is free software. You can redistribute and/or modify it under the terms of the **GNU General Public License (GPL)** as published by the Free Software Foundation.

## 👤 Author

**Amilcar Antonio Mesquita Rizk**
*Copyright © 2026*

---
*Disclaimer: This software is provided "as is", without warranty of any kind. Use for astrological research and professional practice at your own discretion.*

***
