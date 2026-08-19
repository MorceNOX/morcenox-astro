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

#ifndef ARABIC_PARTS_H
#define ARABIC_PARTS_H

#include "helper.h"
#include "planet_table.h"


typedef struct {
    int id;
    char name[64];
    int gender_id; // Mudado de 'char' para 'int' (1, 2 ou 3)
    int personal_point;
    int significator;
    int trigger;
    char description[256];
} ArabicPartFormula;

typedef struct {
    char name[50];
    double longitude;
    char house[8];
    char lord[8];
    char link[10];
    char description[256];
} ArabicPartCalculada;

typedef struct {
    bool has_aspect;
    char symbol[8];  // ☌, ⚹, □, △, ☍
    int color_pair;   // Armazena a cor correspondente à natureza do aspecto
} PartAspectCell;

typedef struct {
    // Linhas = Planetas (Até 12: 7 Tradicionais + 3 Modernos + 2 Nodos)
    // Colunas = Partes Árabes (Até 50 registros carregados do SQLite)
    PartAspectCell grid[12][50]; 
} AspectPartMatrix;



#define MAX_PARTS 100


void get_part_abbreviation(char *name, char *abreviacao);
double get_longitude_by_id(int id, int num_objects, ChartObject *obj);
int load_and_calculate_arabic_parts(ChartObject *obj, int num_objects, double *cusps, ArabicPartCalculada *lista_resultado);
void display_arabic_parts(ChartObject *obj, double *cusps, int num_objects);
void display_arabic_parts_solar_natal_confrontation(ChartObject *obj, double *cusps, int num_objects, double *cusps_natal);
int obter_id_parte_por_nome(const char *name_lote);

void form_arabic_part(ChartObject *obj, int num_objects, int part_id_edicao);
void calcular_aspectos_partes(ChartObject *obj, int num_objects, ArabicPartCalculada *lista, int qtd_partes, AspectPartMatrix *m_part);
void display_part_aspects(ChartObject *obj, int num_objects, ArabicPartCalculada *lista, int qtd_partes);
void deletar_parte_arabe_com_confirmacao(int id_banco_alvo, const char *nome_parte);


#endif