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

#ifndef DRAW_CHART_H
#define DRAW_CHART_H

#include <time.h>
#include "helper.h"
#include "planet_table.h"
#include "aspects.h"
#include "mind.h"


#define P_SOL 0
#define P_LUNA 1
#define P_MERCURY 2
#define P_VENUS 3
#define P_MARS 4
#define P_JUPITER 5
#define P_SATURN 6

#define P_URANUS 7
#define P_NEPTUNE 8
#define P_PLUTO 9

#define P_NORTH_NODE 10
#define P_SOUTH_NODE 11
#define P_FORTUNA 12
#define P_SAN 13

#define P_ASC 14
#define P_MC 15
#define P_DC 16
#define P_IC 17
#define P_VERTEX 18

#define SAN_CONJUNCIONAL 1
#define SAN_PREVENCIONAL 2

#define OBJ_PLANET 1
#define OBJ_ANGLE 2
#define OBJ_POINT 3
#define OBJ_ARABIC_PART 4
#define OBJ_ZODIAC_DEGREE 5
#define OBJ_CUSP 6
#define OBJ_FIXED_STAR 7
#define OBJ_DISPOSITOR 8

#include "directions.h"

// Estrutura para mapear cada fatia do termo
typedef struct {
    int grau_limite; // O grau onde o termo TERMINA dentro do signo
    int regente;     // Índice do planeta regente no array de símbolos
} Termo;


typedef struct {
    double julian_day;
    struct tm *local_time;
    double lat;
    double lon;
    double elev;
    PlotObject *plots;
    char *season_fmt;
    int sanYear;
    int sanMon;
    int sanDay;
    double sanHour;
    char *sunrise_time;
    char *sunset_time;
    char *next_sunrise_time;
    char *city;
    char *country;
    const char *phase;
    char *moon_temperament;
    bool dark_mode;
    int last_hr;
    int last_min;
    double last_sec;
    char *chart_name;
    int gender_id;
    
    PlanetTableMatrix planet_matrix;
    PlanetDignities *dig;
    
    AspectMatrix matrix;
    DeclMatrix matrix_decl;
    
    int week_day;
    double *hours;
    int planetary_hour;
    double daytime_hour;
    double nighttime_hour;
    
    double *cusps;
    char **pHouse;
    char **house_ruler_str;
    char *house_system_str;
    
    PontosHylegiacos pontos_calculados;
    
    int phase_id;
    int season_id;
    
    int anos_alcochoden;
    
    int signo_da_casa_8;
    int regente_dia;
    int regente_hora;
    
    ChartObject *obj;
    int total_objects;
    
    bool animated;
    int anim_interval;

    DadosPlanetaMente mercurio;
    DadosPlanetaMente lua;
    int mercury_retro;

    float zoom_factor;
    float pan_x;
    float pan_y;
    int n;
    double tz_offset;
    bool house_div;
    char house_system;

    bool mapa_retorno;
    int qtd_almuten_rev;
    int *almuten_rev;

    double almuten_lon;
    int dig_almuten_natal;
    double almuten_lat;
    double armc;
    double ascendant;
    double lat_natal;
    int senhor_da_profeccao;
    int id_senhor_firdaria;
    int id_senhor_subfirdaria; 
    double armc_natal;

    char *nome_anareta;
    char *nome_senhor_da_casa8;
    int tipo_h;
    int idx_objeto_h;
    double *planet_longitudes;
    double jd_natal;
    double *planet_latitudes;
    double *longitudes_natal;

    int *strength_planets;

    int *strength_natal;

    int tipo_h_natal;
    int idx_hyleg_natal;
    double asc_natal;

    double *cusps_natal;

    int tipo_san;

    Promissor *prom;

    AntObject *ants;

    int *house_rulers;

    ChartObject *obj_natal;
    int total_obj_natal;

} ContextoMenu;

void open_menu_tables(ContextoMenu *ctx);

const char **get_planet_ascii(int planet_id);
const char **get_planet_ascii_by_name(char *planet_name);
const char **get_planet_ascii_by_gliph(char *planet);

void get_terms_longitude_to_print(Termo tabela_termos[12][5], Termo t_lon[12][5]);

int obter_idade_padrao_mapa();
double obter_idade_padrao_mapa_double();
const char* obter_glifo_planeta_por_id(int id_planeta);
const char* obter_nome_planeta_por_id(int id_planeta);
int converter_codigo_planeta(int codigo_antigo);
int get_planetary_joy(int id_planet);
int get_sign_joy(int id_planet);
const char* get_house_roman(double longitude, double *cusps);
int get_house(double longitude, double *cusps);
const char* get_moon_phase_name(double elongation);
int get_moon_phase_id(double elongation);
int get_moon_quarter(double elongation);
double get_moon_phase_by_longitude(double julian_day);

int get_hour_regent(int week_day, int planetary_hour);
int get_term_ruler(double longitude_total);
int get_decan(double longitude_total);
int get_decan_ruler(int ndec);

double get_sun_altitude(double tjd_ut, double lat, double lon, double elev);
double find_sun_event(double jd_start_midnight_ut, double lat, double lon, double elev, bool is_sunrise);
struct tm julian_day_para_struct_tm(double jd_retorno);
double normalize360(double val);
double normalize_angle(double angle);
double get_sun_moon_diff(double jd);
double find_last_astrological_event(double start_jd, bool is_opposition);

int chart(struct tm *local_time, double lat, double lon, double elev, double tz_offset, char *city, char *country, bool animated, int anim_interval, char *chart_name, char house_system, int gender_id, int darkmode, int mapa_retorno, int senhor_da_profeccao, int id_senhor_firdaria, int id_senhor_subfirdaria, double armc_natal, double lat_natal, PlanetDignities *dig_natal, char *nome_anareta_natal, char *nome_s8_natal, int tipo_h_natal, int idx_hyleg_natal, double *longitudes_natal, double jd_natal, int *strength_natal, double asc_natal, double *cusps_natal, ChartObject *obj_natal, int total_obj_natal);
void draw_circle_points(int center_y, int center_x, float radius, float aspect_ratio, float current_scale, const wchar_t* character);
void draw_circle_points_delay(int center_y, int center_x, float radius, float aspect_ratio, float current_scale, const wchar_t* character, int delay_ms, bool clockwise);
void draw_circle_outline(int center_y, int center_x, float radius, float aspect_ratio, float current_scale, const wchar_t* character);
void draw_circle_filled(int center_y, int center_x, float radius, float aspect_ratio, float current_scale, const wchar_t* character);
void draw_objects_at_radius(int radius_multiplier, int object_count, PlotObject *plots, int n, int display_center_y, int display_center_x, float current_scale, float aspect_ratio, int asc, double *cusps);
void draw_cusps(int radius_multiplier, int object_count, double *cusps, int n, int display_center_y, int display_center_x, float current_scale, float aspect_ratio);
void draw_cusps_div(int object_count, double *cusps, int n, int display_center_y, int display_center_x, float current_scale, float aspect_ratio);
void draw_day_hour_regents(int week_day, int planetary_hour, int display_center_y, int display_center_x, float current_scale, float aspect_ratio);
void draw_zodiac_signs(int display_center_y, int display_center_x, float current_scale, float aspect_ratio, int n, int asc);
void draw_decans(int display_center_y, int display_center_x, float current_scale, float aspect_ratio, int n, int asc);

void draw_chart(float aspect_ratio, float zoom_factor, float pan_x, float pan_y, 
    int n, struct tm *local_time, double lat, double lon, double elev, double tz_offset,
    PlotObject *plots, double *cusps, int sanYear, int sanMon, int sanDay, double sanHour, 
    char *sunrise_time, char *sunset_time, char *city, char *country, 
    double daytime_hour, double nighttime_hour, int week_day, int planetary_hour, 
    const char* phase, bool dark_mode, bool animated, int anim_interval, bool mapa_retorno,
    char *chart_name, char house_system, int gender_id, bool house_div, int last_hr, int last_min, double last_sec, bool show_dec, Termo terms[12][5], bool show_terms);


int get_opposite_sign(int sign);
int get_sign_antiscium(int sign);
double get_antiscium_degree(double degree);

char *get_sign(int n);
char *get_sign_name(int n);
char *get_sign_element(int n);
char *get_sign_element_name(int n);
bool mapa_diurno();
int diff_sign(int signA, int signB);

void calcular_forca_planetas(PlanetDignities *dig, int *resultado_strength, bool com_modernos);
double get_total_ponderado(double essential, double accidental);
double get_total_normalized(double essential, double accidental);
double get_planet_force(int id, double total_normalized, double *weights);
void display_force(PlotObject *plots, PlanetDignities *dig, int *strength_planets);
void display_planetary_energy_profile(PlotObject *plots, int *strength_planets);
void display_dignities(PlotObject *plots, PlanetDignities *dig, int *strength_planets);

void display_table_data(bool mapa_retorno, double jd, struct tm *local_time, double lat, double lon, double elev, PlotObject *plots, char *season,
    int sanYear, int sanMon, int sanDay, double sanHour, char *sunrise_time, char *sunset_time, char *next_sunrise_time,
    char *city, char *country, const char* phase, char *temperament,
    int last_hr, int last_min, double last_sec, char *chart_name, int gender_id);

void display_table(PlotObject *plots, PlanetTableMatrix *matrix, PlanetDignities *dig, int *strength_planets);

void display_houses(double *cusps, char pHouse[12][100], char **house_ruler, char *house_system);

void display_hours(int week_day, double *hours, int planetary_hour, double daytime_hour, double nighttime_hour, int *strength_planets, PlanetDignities *dig);
void abrir_janela_interpretacao_horas(int regente_dia, int regente_hora, const char *regent_day_str, const char *regent_hour_str, int strength_reg_day, int strength_reg_hour, int dig_reg_day, int dig_reg_hour);

void display_rising_times(PlotObject *plots, double tz_offset);

double get_hours_from_jd(double jd, double offset);
void format_event_time(double jd, double offset, char *dest, size_t size_of_dest);
double calc_solar_time(double julian_day, double ut_hours, double lon);
struct tm julian_day_para_struct_tm(double jd_retorno);
double calc_declination_mathematical_point(double jd, double longitude);

#endif
