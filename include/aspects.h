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

#ifndef ASPECTS_H
#define ASPECTS_H

#include "planet_table.h"

typedef struct { 
    double angle; 
    char *symbol; 
    char *name; 
} AspectDefs;


// Representa uma célula ativa na grade de aspectos
typedef struct {
    bool has_aspect;         // Indica se existe aspecto válido nesta combinação
    char symbol[10];          // Símbolo Unicode do aspecto (ex: "☌", "□", "▓▓▓")
    int color_pair;          // Par de cor Ncurses pré-calculado para o aspecto
    double angle;            // O ângulo real ou a distância (orbe) para exibição
    bool is_bold;
    bool is_reverse;           // Flag para destacar aspectos maiores (quadratura/oposição)
    bool is_aplicative;
    bool is_partil;            
} AspectCell;

// A matriz completa que será enviada para a função de desenho
typedef struct {
    AspectCell grid[NUM_OBJECTS][NUM_OBJECTS];
} AspectMatrix;

typedef struct {
    bool has_aspect;       // Indica se há paralelo ou contra-paralelo válido
    char symbol[10];        // Símbolo Unicode do aspecto ("∥", "∦")
    bool is_reverse;
    int color_pair;        // Par de cor Ncurses injetado
    double diff;           // A diferença residual do orbe para exibição (%3.1f)
} DeclCell;

typedef struct {
    DeclCell grid[NUM_OBJECTS][NUM_OBJECTS];
} DeclMatrix;

double ASP_ANTISSIA_EXACT();
double ASP_PARALLEL_EXACT();

bool has_aspect(int id1, int id2, AspectMatrix *matrix);
bool has_aspect_aplicative(int id1, int id2, AspectMatrix *matrix);
bool has_aspect_partil(int id1, int id2, AspectMatrix *matrix);
bool has_aspect_aplicative_or_partil(int id1, int id2, AspectMatrix *matrix);
bool has_aspect_separative(int id1, int id2, AspectMatrix *matrix);

bool is_under_siege(int planet_id, AspectMatrix *matrix);
bool is_under_assistance(int planet_id, AspectMatrix *matrix);

AspectMatrix calculate_aspects(PlotObject *plots, double *planet_orbis, PlanetDignities *dig, int *feral, int *vazio_de_curso, int *retro);
DeclMatrix calculate_declination_aspects(PlotObject *plots, double decl_orbis);
void display_aspects(PlotObject *plots, AspectMatrix *matrix, DeclMatrix *matrix_decl, AntObject *ants, int num_ants);
void display_declination_aspects(PlotObject *plots, DeclMatrix *matrix);
AspectMatrix calculate_aspects_by_sign(PlotObject *plots);
void display_aspects_by_sign(PlotObject *plots, AspectMatrix *matrix);
AspectMatrix calculate_aspects_antiscium(PlotObject *plots, AntObject *ants, int num_objects);
void display_aspects_antissium(PlotObject *plots, AntObject *ants, int num_ants, AspectMatrix *matrix);
#endif
