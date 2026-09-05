/*
 * MorceNOX-ASTRO™
 * Copyright (C) 2026 Amilcar Antonio Mesquita Rizk amilcar.rizk@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://gnu.org>.
 */

#ifndef VAR_H
#define VAR_H

#include <sqlite3.h>
#include <libintl.h>

#ifndef LOCALEDIR
#define LOCALEDIR "./locale"
#endif

//#define _(String) gettext(String)
#define _(String) dgettext(LANGUAGE, String)

#define MAX_AGE 150.0

extern sqlite3 *global_db;

extern char LANGUAGE[8];


extern char DEFAULT_CITY[100];
extern char DEFAULT_COUNTRY[100];
extern char DEFAULT_STATE[100];
extern char DEFAULT_TZ_IANA[100];
extern double DEFAULT_TZ_OFFSET;

extern char CITY[100];
extern char COUNTRY[100];
extern char STATE[100];
extern char TZ_IANA[100];

extern char CHART_NAME[100];
extern char MESSAGE[100];
extern double TZ_OFFSET;
extern double DST_OFFSET;

extern int YY, MM, DD, HH, MIN, SEC, DST;

extern int DARK_MODE;
extern char HOUSE_SYSTEM;

extern char CONFIG_PATH[256];
extern const char *DB_PATH;

extern bool MAPA_DIURNO;
extern int GENDER;

extern bool show_modern_planets;
extern bool consider_modern_planets_rulling;

extern int terms_system;

extern int FLAGS;

extern const char *planet_regent_symbols[7];
extern const char *planet_regent_names[7];

extern const int MAX_HELP_LINE_WIDTH;
extern const int MAX_LINE_WIDTH;

extern const double ANTISCIUM_ORB;
extern const double ASP_MAJOR_EXACT;

void update_interface_language();

#endif
