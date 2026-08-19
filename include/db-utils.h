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

#ifndef DB_UTILS_H
#define DB_UTILS_H

#include <sqlite3.h>
#include "selections.h"
#include "planet_table.h"
#include "helper.h"



sqlite3* open_database();
void close_database(sqlite3 *db);
void terminate_database();
int reconstruir_indice_cidades(sqlite3 *db);
int configurar_banco(sqlite3 *db);
void auto_vacuum(sqlite3 *db);
int execute_query(sqlite3 *db, const char *sql, sqlite3_stmt **stmt);
int prepare_statement(sqlite3 *db, const char *sql, sqlite3_stmt **stmt);
int execute_with_database(const char *sql, int (*callback)(sqlite3*, sqlite3_stmt*));
void free_string_array(char **array, int count);
int get_coordinates(char *city, char *country, char *state, double *lat, double *lon, double *elev, double *tz_offset, char *tz_iana);
int get_sign_properties(int sign, PrimitiveProperties *props);
int get_planet_properties(int id, PrimitiveProperties *props);
void get_cities_from_db(const char *country, char ***cities, char ***states, int *count);
void get_countries_from_db(char ***countries, int *count);
int get_planet_orbis(int planet_ids[], double planet_orbis[], char planet_symbols[][10], int max_planets);
int get_rulers(char *sign_symbol, int *n_ruler, int *n_exalted, int *n_exile, int *n_fall, int *n_tri1, int *n_tri2, int *n_tri3); 
int get_planet_gender(char *planet_symbol, int *gender_id);
int get_rulers_by_lon(double longitude, bool consider_modern_planets_rulling, int *n_dom, int *n_exalted, int *n_tri1, int *n_tri2, int *n_tri3, int *n_term, int *n_dec);
int get_rulers_by_sign_id(int sign, int *n_ruler, int *n_exalted, int *n_exile, int *n_fall, int *n_tri1, int *n_tri2, int *n_tri3);
int get_ruler_dom_by_sign_id(int sign, int *n_ruler);  
int get_sign_gender(char *sign_symbol, int *gender_id);
int get_quadrant_gender(int house, int *gender_id);
int get_planet_sect(char *planet_symbol, int *sect_id);
int get_sign_ruler_by_domicile(char *sign_symbol, char **ruler_symbol);
int get_sign_ruler_by_exaltation(char *sign_symbol, char **ruler_symbol, int consider_modern_planets);
int get_season(char *sign_symbol, char **season);
int get_season_id(int sign, int *season);
int get_season_temperament(char *season, char **temperament);
int get_season_temperament_by_season_id(int season_id, char **temperament);
int get_planet_speed(char *planet_symbol, double *speed);
int get_weights(double weights[50], bool show_modern_planets);
int get_chart_objects(ChartObject obj[100]);
int get_planet_orientality(char *planet_symbol, char **orientality);
int get_house_system_name(char house_system_id, char **house_system_name);
int get_default_gender();
int get_default_terms_system();
int update_triplicity_rulers(int triplicity_system);
int update_settings(ChartOptions options);
int get_moon_temperament(const char *phase, char **temperament);
int get_moon_temperament_by_phase_id(int phase_id, char **temperament);
int get_house_meaning(int house, char *meaning);

#endif
