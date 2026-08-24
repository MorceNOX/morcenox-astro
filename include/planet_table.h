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

#ifndef PLANET_TABLE_H
#define PLANET_TABLE_H

#define NUM_OBJECTS 19
#define NUM_OBJECTS_EXT 38

typedef struct {
    int id;
    char object[10];
    char object_name[30];
    double longitude;
    double latitude;
    double declination;
    int house;
} AntObject;


typedef struct {
    int id;
    double longitude;
    char *object;
    char *object_name;
    char *sign;
    char *degree;
    char *min;
    char *retrograde;
    char *house;
    double speed;
    double rising_time;
    double setting_time;
    double mid_time;
    double declination;
} PlotObject;

typedef struct {
    int id;
    double longitude;
    int house;
    int type;
    char object[10];
    char object_name[30];
    int object_ref;
} ChartObject;

typedef struct {
    char decl_str[15];       // Declinação formatada
    char decan[10];
    char term[10];
    char tri[30];
    char speed_str[15];      // Velocidade formatada
    int speed_color_pair;    // Cor da velocidade (8 para rápido, 11 para lento, 0 para neutro)
    int house_color_pair;    // Cor baseada na classificação da Casa
    char dignity_str[80];    // Texto da dignidade essencial (ex: "Dom/Exalt/Tri")
    int dignity_color_pair;  // Cor da dignidade essencial
    bool gender_match;       // Flag para exibir o "✅" de conformidade de gênero
    bool sect_match;         // Flag para exibir o "✅" de conformidade de seita (Sect)
    bool quadrant_match;     // Flag para exibir o "✅" de conformidade de quadrante (gênero)
    char orientality_str[30];
    int orientality_color_pair; 
    char rulers_str[30];     // String combinada dos regentes por domicílio e exaltação
    char mutual_reception[32];
} PlanetRowData;

typedef struct {
    PlanetRowData rows[NUM_OBJECTS];
} PlanetTableMatrix;


typedef struct {
    int pilgrim;
    int movement;
    int fast;
    int feral;
    int void_of_course;
    int orientality;
    int combust;
    int under_rays;
    int cazimi;
    int under_siege;
    int under_assistance;
    int asp_benef_conj;
    int asp_benef_trine;
    int asp_benef_sextile;
    int asp_malef_conj;
    int asp_malef_opp;
    int asp_malef_square;
    int north_node_conj;
    int south_node_conj;
    int haym;
    int hayz;
    int hayz_extra;
    int joy;
    int sign_joy;
    int mut_reception;
    int mut_reception_asp;
} AccidentalDignities;




typedef struct {
    int id;
    int essential;
    int accidental;
    int major_dig;
    AccidentalDignities row;
} PlanetDignities;



// Estrutura com os dados hylegíacos pré-calculados que você já possui
typedef struct {
    double sol_lon;
    double lua_lon;
    double asc_lon;
    double fortuna_lon;
    double sizigia_lon;
} PontosHylegiacos;


typedef struct {
    int temperature;
    int moisture;
} PrimitiveProperties;

#define DOMICILE 2
#define EXALTATION 1
#define EXILE -2
#define FALL -1
#define NO_MAJOR_DIGNITY 0

#define HOUSE_1_PT 5
#define HOUSE_2_PT 3
#define HOUSE_3_PT 2

#define HOUSE_4_PT 4
#define HOUSE_5_PT 3
#define HOUSE_6_PT -3

#define HOUSE_7_PT 4
#define HOUSE_8_PT -4
#define HOUSE_9_PT 3

#define HOUSE_10_PT 5
#define HOUSE_11_PT 4
#define HOUSE_12_PT -5



#endif
