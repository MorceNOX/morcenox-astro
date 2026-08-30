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

#include <stddef.h>
#include <sqlite3.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libintl.h>

#include "var.h"


char LANGUAGE[8] = "en"; 

char DEFAULT_CITY[100] = "Guarulhos";
char DEFAULT_COUNTRY[100] = "Brazil";
char DEFAULT_STATE[100] = "São Paulo";
char DEFAULT_TZ_IANA[100] = "America/Sao_Paulo";
double DEFAULT_TZ_OFFSET = -3.0;
char CITY[100] = "Guarulhos";
char COUNTRY[100] = "Brazil";
char STATE[100] = "São Paulo";
char TZ_IANA[100] = "America/Sao_Paulo";

char CHART_NAME[100] = "NewChart";
char MESSAGE[100] = "";
double TZ_OFFSET = -3.0;
double DST_OFFSET = 0.0;

int YY = 2026, MM = 6, DD = 6, HH = 12, MIN = 0, SEC = 0, DST = -1;

int DARK_MODE = 0;
char HOUSE_SYSTEM = 'P';

char CONFIG_PATH[256];
const char *DB_PATH;

sqlite3 *global_db = NULL;

bool MAPA_DIURNO;
int GENDER = 3;

bool show_modern_planets;
bool consider_modern_planets_rulling;

int terms_system = 0;

int FLAGS = 0;

const char *planet_regent_symbols[7] = {"☉", "♀", "☿", "☽", "♄", "♃", "♂"};
const char *planet_regent_names[7] = {"Sun", "Venus", "Mercury", "Moon", "Saturn", "Jupiter", "Mars"};


// void update_interface_language() {
//     setenv("LANGUAGE", LANGUAGE, 1);
//     setenv("LC_ALL", LANGUAGE, 1);

//     setlocale(LC_ALL, "");

//     bindtextdomain("astro", NULL); 
    
//     bindtextdomain("astro", LOCALEDIR); 
//     bind_textdomain_codeset("astro", "UTF-8");
    
//     textdomain("astro");
// }

void update_interface_language() {
    // 1. Atualiza as variáveis de ambiente para o sistema operacional
    setenv("LANGUAGE", LANGUAGE, 1);
    setenv("LC_ALL", LANGUAGE, 1);
    setlocale(LC_ALL, "");

    // 2. Limpa o cache do gettext desvinculando o domínio antigo antes de registrar o novo
    bindtextdomain(LANGUAGE, NULL); 
    
    // 3. Vincula o domínio (pt ou en) à pasta de traduções oficial
    bindtextdomain(LANGUAGE, LOCALEDIR); 
    bind_textdomain_codeset(LANGUAGE, "UTF-8");
    
    // 4. Define o domínio ativo (faz o gettext buscar por pt.mo ou en.mo)
    textdomain(LANGUAGE);
}
