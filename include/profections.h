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

#ifndef PROFECTIONS_H
#define PROFECTIONS_H

typedef struct {
    int casa_ativada;       // 1 a 12
    int signo_ativado;      // 1 a 12 (1=Áries, 2=Touro...)
    int id_senhor_do_ano;   // 1 a 7 
    char glifo_senhor[10];  // "☉", "☽", "♃", etc.
    char nome_senhor[20];   // "Sun", "Moon", "Jupiter", etc.
} DadosProfeccao;

DadosProfeccao calcular_profeccao_anual(double asc_longitude, int idade_atual);
void display_profections(PlotObject *plots, int anos_alcochoden);

#endif