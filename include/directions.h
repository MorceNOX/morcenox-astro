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

#define PROM_PLANET 1
#define PROM_TERM 2
#define PROM_POINT 3

typedef struct {
    int id;
    char object[10];
    char object_name[30];
    double longitude;
    double latitude;
    double declination;
    int house;
    int type;
} Promittor;

typedef struct {
    char promissor_name[20]; // O planeta que se move (ex: "Mars")
    char promissor_glifo[10];
    int promittor_type;
    char aspecto_symbol[10]; // ☌, ⚹, □, △, ☍
    char significador_name[20]; // O ponto receptor parado (ex: "Moon" [Hyleg])
    char significador_glifo[10];
    double arco_graus;       // Distância em graus no Equador Celeste
    double idade_evento;     // Idade calculada via Naibod
    int ano_calendario;      // Ano exato do evento (YY + idade)
    int mes_calendario;
    int dia_calendario;
    char tipo_direcao[15];   // "Zodiacal" ou "Mundane"
    int sentido;        // 0 = Direct; 1 = Converse
} LinhaDirecao;

double calcular_semi_arco(double dec_rad, double lat_geografica_rad, int acima_do_horizonte);
double calcular_distancia_meridiana(double ra, double ramc, int acima_do_horizonte);
double calcular_ra(double longitude, double declinacao, double jd);
int calcular_direcoes_zodiacais_geral(PlotObject *plots, int idx_alvo, LinhaDirecao *lista_resultado, double jd, double *latitudes, int sentido, Promittor *prom);
void display_primary_directions(PlotObject *plots, AspectMatrix *matrix, PontosHylegiacos pontos, int regente_dia, int regente_hora, char *nome_anareta, char *nome_senhor_da_casa8, int tipo_h_natal, int idx_hyleg_natal, bool mapa_retorno, double jd, double *latitudes, int tipo_san, PlanetDignities *dig, double ramc, double lat, Promittor *prom);
void display_primary_directions_parts(Promittor *prom, char *nome_anareta, char *nome_senhor_da_casa8, ChartObject *obj, int num_objects, double *cusps, double jd, double *latitudes, double ramc, double lat);
int calcular_direcoes_zodiacais_partes(ArabicPartCalculada *parts, int qtd_partes, int idx_alvo, LinhaDirecao *lista_resultado, double jd, double *latitudes, int sentido, Promittor *prom);
int calcular_direcoes_mundanas_geral(PlotObject *plots, int idx_alvo, LinhaDirecao *lista_resultado, double jd, double ramc, double lat_geografica, int sentido, Promittor *prom);
int calcular_direcoes_mundanas_partes(ArabicPartCalculada *parts, int idx_alvo, LinhaDirecao *lista_resultado, double jd, double ramc, double lat_geografica, int sentido, Promittor *prom);
#endif