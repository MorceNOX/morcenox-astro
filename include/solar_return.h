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

#ifndef SOLAR_RETURN_H
#define SOLAR_RETURN_H

void processar_confronto_natal_revolucao(
    int id_almuten_rev,               
    double longitude_almuten_rev,     
    double latitude_almuten_rev,      
    int dignidade_natal,           
    double lat_natal,                 
    double armc_natal,                
    int id_senhor_profeccao,          
    int id_senhor_firdaria,           
    int id_senhor_subfirdaria,
    double asc_revolucao,
    int *strength_planets,
    double jd_natal,                  /* ADICIONADO: JD real do Nascimento */
    double jd_revolucao_ut,
    double armc_rev,
    double lat_rev,
    double asc_natal);           /* ADICIONADO: ASC calculado para a cidade do aniversário */


struct tm obter_tempo_local_revolucao(double jd_revolucao_ut, double fuso_horario_destino);
double calcular_julian_day_retorno_solar(double jd_nascimento, int idade_selecionada, double sol_natal_exibido);
double calc_julian_day_retorno_solar(double jd_nascimento, int idade_selecionada);
void disparar_revolucao_solar(double julian_day, char *chart_name, double *cusps_natal, bool mapa_diurno, double lat, double armc, PlanetDignities *dig, char *nome_anareta_natal, char *nome_s8_natal, int tipo_h_natal, int idx_hyleg_natal, double *longitudes_natal, int *strength_planets, ChartObject *obj_natal);

void abrir_janela_confronto_natal_revolucao(
    int id_almuten_rev, 
    int pontuacao_dignidade_natal, 
    int casa_natal_transitada, 
    int id_senhor_profeccao, 
    int id_senhor_firdaria, 
    int id_senhor_subfirdaria,
    int casa_natal_do_asc,
    int aproveitamento_almuten,
    int casa_rev_do_asc_natal);

void get_natal_houses_rev_planets(double jd_natal, double *rev_longitudes, double *rev_latitudes, double armc_natal, double lat_natal, char house_system, int *casas_planetas_natal_proj);
void get_hyleg_data(int tipo_h_natal, int idx_hyleg_natal, double *longitudes_natal, double *lon_hyleg_radix_real, char *nome_hyleg_texto, char *glifo_hyleg_texto);
void process_revolution_transits(double jd_natal, double *rev_longitudes, double *rev_latitudes, double armc_natal, double lat_natal, char house_system, int tipo_h_natal, int idx_hyleg_natal, double *longitudes_natal);
void abrir_janela_transitos_revolucao(
    double *longitudes_rev, 
    double *longitudes_natal, 
    int *casas_planetas_rev,
    double lon_hyleg_natal,       /* ADICIONADO: Longitude do Hyleg do Radix */
    char *nome_hyleg_natal,       /* ADICIONADO: Nome do Hyleg (ex: "Jupiter", "Ascendant") */
    char *glifo_hyleg_natal);

#endif