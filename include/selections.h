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

#ifndef SELECTIONS_H
#define SELECTIONS_H

#include "number_helper.h"

// Estrutura para consolidar a data de nascimento
typedef struct {
    int ano;
    int mes;
    int dia;
} DataNascimento;

typedef struct {
    DataNascimento date;
    int changed;
} DateEdition;

typedef struct {
    Hora hora;
    int changed;
} HoraEdition;

typedef struct {
    int dark_mode;
    char house_system;
    int triplicity_system;
    int terms_system;
    int modern_planets_rulling;
    int show_modern_planets;
    int gender;
    char language[10];
} ChartOptions;

typedef struct {
    ChartOptions options;
    int changed;
} OptionsEdition;

int show_confirm_yesno(const char *name, const char *text);
int show_confirm_delete_popup(const char *name);
void show_alert_popup(const char *txt_line1, const char *txt_line2);
ChartOptions load_default_options();
OptionsEdition select_options();
int set_tz();
int set_dst();
void set_chart_name(char *chart_name, size_t max_length);
int selecionar_idade_visual(int idade_inicial);
double selecionar_idade_visual_fracionada(double idade_inicial);
int select_gender();
DateEdition selecionar_data();
HoraEdition selecionar_hora();
int *get_topics_grep(char ***lines, int *line_count, char *file);
int select_topic(char *file);
int load_city_coordinates(char *city_chart, char *country_chart, char *state_chart, char *tz_iana_chart, double *tz_offset_chart, double *lat, double *lon, double *elev);
void set_default_city();

#endif
