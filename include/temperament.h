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

#ifndef TEMPERAMENT_H
#define TEMPERAMENT_H

#define COLERICO   0
#define SANGUINEO  1
#define MELANCOLICO 2
#define FLEUMATICO  3

#define TEMPERAMENT_RANK_PROPORTION 0.666666667

/* Estrutura para armazenar as pontuações brutas acumuladas dos planetas e pontos */
typedef struct {
    int total_quente;
    int total_frio;
    int total_umido;
    int total_seco;
} ScoreTemperament;



/* Estrutura utilizada para o ranking e ordenação (Bubble Sort) dos humores */
typedef struct {
    int id;
    char label[30];    /* Armazena o nome e o símbolo do elemento (ex: "Choleric (🜂 Fire):") */
    float porcentagem; /* Valor percentual calculado (0.0f a 100.0f) */
    int cor_par;       /* O ID do par de cores do ncurses associado ao temperamento */
} ItemTemperamento;



void desenhar_barra_porcentagem(WINDOW *win, int row, int col, float porcentagem, int cor_par);
void desenhar_barra_temperamento(WINDOW *win, int row, int col, int valor, int total, int cor_par);
void display_temperament(PlotObject *plots, AspectMatrix *aspecto_matrix, int fase_lunar, int estacao, int week_day, int planetary_hour);
void abrir_janela_interpretacao_temperamento(ScoreTemperament score, ItemTemperamento *lista, int eixo_calor, int eixo_umidade);

#endif