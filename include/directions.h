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

#ifndef DIRECTIONS_H
#define DIRECTIONS_H

#include "arabic_parts.h"
#include "aspects.h"
#include "var.h"

#define PROM_PLANET 1
#define PROM_TERM 2
#define PROM_POINT 3
#define PROM_ANTISCIUM 4
#define PROM_CONTRANTISCIUM 5
#define PROM_CUSP 6
#define PROM_ANGLE 7
#define PROM_PART 8


#define DIRECT 0
#define CONVERSE 1

#define TOTAL_SIGNIFICADORES 32

//#define NAIBOD_KEY  1.014561666311 //1.014646  // Chave de Naibod: graus equatoriais por ano de vida
#define NAIBOD_KEY   0.985647358005769
#define CARDANO_KEY   0.986666679822222
#define PTOLEMY_KEY  1.000000000000000
#define PLACIDUS_KEY 0.986388888888889

#define DELTA_ARCO 0.00277778  // Tolerância de 10 segundos de arco (equivalente a 1 dia de vida)
                    
typedef enum {
    DIRECAO_ZODIACAL = 0,
    DIRECAO_MUNDANA  = 1
} TipoDirecaoEnum;


#define RAD_TO_DEG (180.0 / M_PI)
#define DEG_TO_RAD (M_PI / 180.0)

typedef struct {
    double ra;
    double dec;
    SemiArcos sa;
    double md;
    double hd;
    double pm; // Posição Mundana de Placidus (0° a 360°)
} DadosAstro;


typedef struct {
    int promissor_id;
    char promissor_name[64]; // O planeta que se move (ex: "Mars")
    char promissor_glifo[10];
    int promissor_type;
    int aspecto_id;
    char aspecto_symbol[10]; // ☌, ⚹, □, △, ☍
    char significador_name[20]; // O ponto receptor parado (ex: "Moon" [Hyleg])
    char significador_glifo[10];
    double arco_graus;       // Distância em graus no Equador Celeste
    double idade_evento;     // Idade calculada via Naibod
    int ano_calendario;      // Ano exato do evento (YY + idade)
    int mes_calendario;
    int dia_calendario;
    int tipo_direcao_id;
    char tipo_direcao[15];   // "Zodiacal" ou "Mundane"
    int sentido;        // 0 = Direct; 1 = Converse
    char divisor_name[64];
    char divisor_gliph[10];
    SemiArcos sig_sa;
    double sig_md;
    double sig_hd;
    double sig_pm;
} LinhaDirecao;

SemiArcos calcular_semi_arcos(double latitude_geo, double declinacao);
double get_time_key(int key, double jd, double arco);
const char* get_key_name(int key);
double get_obliquidade(double jd);
double obter_ano_tropical_dinamico(double jd_natal);

void calc_mundane_part(Promissor *sig, Promissor *trig, Promissor *pp, double ramc, double jd, double lat, double *cusps, ArabicPartCalculadaDirections *lot_m);
void calc_mundane_lot(Promissor *sol, Promissor *sig, Promissor *trig, Promissor *pp, bool flip, double ramc, double jd, double lat, double *cusps, Promissor *lot_m);
void calc_mundane_lof(Promissor *sol, Promissor *lua, double ramc, double jd, double lat, double *cusps, Promissor *fortune_m);
void calc_mundane_lot_of_fortune(Promissor *sol, Promissor *lua, double ra_asc, double ramc, double lat, double jd, Promissor *fortune_m);
void carregar_parte_fortuna_mundana(Promissor *sol, Promissor *lua, double jd, double ramc, double lat_geo, Promissor *fortuna_m);
void calcular_coordenadas_equatoriais_reversas(double pm, double ramc, double lat_geo, double *out_ra, double *out_dec);
double calcular_parte_fortuna_mundana(double pm_sol, double pm_lua, int sol_esta_acima);
void calcular_coordenadas_equatoriais_reversas(double pm, double ramc, double lat_geo, double *out_ra, double *out_dec);
void injetar_constantes_mundanas_por_casa(int house_number, Promissor *astro);
int verificar_se_acima_horizonte(double ra_planeta, double dec_planeta_rad, double ramc, double lat_geo_rad);
double __calcular_semi_arco(double dec_rad, double lat_rad, int esta_acima);
double _calcular_semi_arco(double dec_rad, double lat_geografica_rad, int acima_do_horizonte);
double _calcular_distancia_meridiana(double ra, double ramc, int acima_do_horizonte);
double descobrir_idade_por_arco_solar(double tjd_ut_natal, double arco_alvo);
double obter_chave_arco_solar(double tjd_ut_natal, double arco_alvo);
double obter_chave_arco_solar_ultra_fast(double tjd_ut_natal, double arco_alvo);
double calcular_arco_kepler_para_idade(double tjd_ut_natal, double idade_anos);
double descobrir_idade_por_arco_kepler(double tjd_ut_natal, double arco_alvo);
double obter_chave_kepler(double tjd_ut_natal, double arco_alvo);
double obter_chave_solar_birthday_longitude(double tjd_ut_natal, double arco_direcao);
double calcular_ra(double longitude, double declinacao, double jd);
int calcular_direcoes_zodiacais_geral(Promissor *sig, int idx_alvo, LinhaDirecao *lista_resultado, double jd, int sentido, Promissor *prom, bool is_part);
void display_primary_directions(PlotObject *plots, Promissor *sig, AspectMatrix *matrix, PontosHylegiacos pontos, int regente_dia, int regente_hora, char *nome_anareta, char *nome_senhor_da_casa8, int tipo_h_natal, int idx_hyleg_natal, bool mapa_retorno, double jd, int tipo_san, PlanetDignities *dig, double ramc, double lat, Promissor *prom, ChartObject for_m);
void display_primary_directions_parts(Promissor *prom, char *nome_anareta, char *nome_senhor_da_casa8, ChartObject *obj, int num_objects, double *cusps, double jd, double ramc, double lat);
int calcular_direcoes_zodiacais_partes(ArabicPartCalculada *parts, int qtd_partes, int idx_alvo, LinhaDirecao *lista_resultado, double jd, int sentido, Promissor *prom);
int calcular_direcoes_mundanas_geral(Promissor *sig, int idx_alvo, LinhaDirecao *lista_resultado, double jd, double ramc, double lat_geografica, int sentido, Promissor *prom, int *mundane_aspects);
int calcular_direcoes_mundanas_partes(ArabicPartCalculada *parts, int idx_alvo, LinhaDirecao *lista_resultado, double jd, double ramc, double lat_geografica, int sentido, Promissor *prom);
void print_directions_to_csv(LinhaDirecao *dir, int qtd_direcoes, MetodoDirecaoGlobal method, int time_key);
#endif