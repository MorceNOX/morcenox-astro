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

#ifndef ALMUTEN_H
#define ALMUTEN_H

int obter_indice_matriz_dig(int id_almuten);
int get_dig_hyleg_points(int id_planeta, PontosHylegiacos pontos);
int get_governed_points_count(int id_planeta, PlotObject *plots);
int get_almuten(double longitude, int *resultados, AspectMatrix *aspecto_matriz, PontosHylegiacos pontos, PlotObject *plots);
int get_almuten_multiplo(double *longitudes, int qtd_longitudes, int *resultados, AspectMatrix *aspecto_matriz, PontosHylegiacos pontos, PlotObject *plots);
void acumular_dignidades_figuris(double longitude, int *tabela_figuris);
int calcular_almuten_figuris(PontosHylegiacos pontos, PlotObject *plots, AspectMatrix *aspecto_matriz, int regente_dia, int regente_hora, int *resultado_figuris);
void display_almutens(PontosHylegiacos pontos, PlotObject *plots, AspectMatrix *aspecto_matriz, int week_day, int planetary_hour, bool mapa_retorno);
void abrir_janela_interpretacao_almuten(int res_almuten[12], int qtd_vencedores);
void abrir_janela_interpretacao_almuten_revolucao(int res_almuten[12], int qtd_vencedores);
#endif