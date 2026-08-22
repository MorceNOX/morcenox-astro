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

#ifndef HYLEG_H
#define HYLEG_H


// Constantes de retorno para identificar o Hyleg encontrado
#define H_SOL     1
#define H_LUNA    2
#define H_FORTUNA 3
#define H_ASC     4
#define H_ALMUTEN 5 // Se o Hyleg for o Almuten Conjunto, o ID do planeta vencedor será armazenado
#define H_ALMUTEN_SAN 6
#define H_SAN     7


typedef struct {
    char object_name[30]; // Nome do planeta (ex: "Venus", "Moon")
    char glifo[10];       // Glifo Unicode (ex: "♀", "☽")
    int anos_concedidos;  // Anos totais calculados
    char tipo_anos[40];   // "Great", "Medium", "Lesser" ou "Proxy Rule"
    int casa_alcochoden;  // NOVO: A casa física real onde o Alcochoden está (1 a 12)
} ResultadoAlcochoden;


typedef struct {
    char name[20];         // Nome do Anareta eleito (ex: "Mars")
    char glifo[10];        // Glifo Unicode (ex: "♂")
    int id_anareta;        // ID de 1 a 7 do planeta destruidor
    int score_ameaca;      // O peso final acumulado (ex: 130)
    char regra_eleicao[80]; // Texto descritivo da regra que selou a escolha
} ResultadoAnareta;

int obter_regente_tradicional(int id_signo);
bool is_lugar_hylegiaco(int casa);
int get_hyleg(PontosHylegiacos pontos, PlotObject *plots, AspectMatrix *aspecto_matriz, int *id_planeta_almuten, int regente_dia, int regente_hora, int tipo_san, PlanetDignities *dig);
const char* obter_descricao_hileg(int tipo_hileg);
int obter_anos_menores_por_nome(const char *object_name);
ResultadoAlcochoden calcular_alcochoden(int tipo_hileg, int idx_hileg_objeto, AspectMatrix *matrix, PlotObject *plots, PlanetDignities *dig, int regente_dia, int regente_hora, PontosHylegiacos pontos);
void display_life_givers(PontosHylegiacos pontos, PlanetDignities *dig, PlotObject *plots, AspectMatrix *matrix, int week_day, int planetary_hour, int tipo_san);
ResultadoAnareta calcular_anareta(int idx_hileg_grid, AspectMatrix *matrix, PlotObject *plots, PlanetDignities *dig, int signo_casa8);
void display_anareta(PlotObject *plots, AspectMatrix *matrix, PlanetDignities *dig, PontosHylegiacos pontos, int signo_casa8, int week_day, int planetary_hour, int tipo_san);

#endif