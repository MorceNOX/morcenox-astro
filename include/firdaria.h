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

#ifndef FIRDARIA_H
#define FIRDARIA_H


typedef struct {
    int dia;
    int mes;
    int ano;
} DataFirdaria;

typedef struct {
    int id_major;
    char glifo_major[10];
    char nome_major[20];
    DataFirdaria inicio_major;
    DataFirdaria fim_major;

    int id_sub;
    char glifo_sub[10];
    char nome_sub[20];
    DataFirdaria inicio_sub;
    DataFirdaria fim_sub;
} RelatorioFirdaria;


int checar_alerta_anaretico(int id_anareta, int id_major_firdaria, int id_sub_firdaria);
RelatorioFirdaria processar_dados_firdaria(double idade_fracao, bool mapa_diurno);
void display_firdaria(PlotObject *plots, AspectMatrix *matrix, PlanetDignities *dig, PontosHylegiacos pontos, int signo_casa8, int regente_dia, int regente_hora);

#endif