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

#ifndef MIND_H
#define MIND_H

typedef struct {
    int id_planeta;             // 1=Sol, 2=Lua, 3=Mercúrio, 4=Vênus, 5=Marte, 6=Júpiter, 7=Saturno
    int total_essencial;
    int total_acidental;
    int esta_combusto;          
} DadosPlanetaMente;

typedef struct {
    char governador_nome[20];
    char condicao_intelecto[100];
    char condicao_emocional[100];
    char perfil_psicologico[256];
    int cor_governador;
} ResultadoMente;


void calcular_qualidades_mente(DadosPlanetaMente mercurio, DadosPlanetaMente lua, int *res_almuten, int total_vencedores, ResultadoMente *res);
void display_natal_mind_analysis(
    DadosPlanetaMente mercurio,
    DadosPlanetaMente lua,
    int mercurio_retrogrado, // 1 se retrógrado, 0 se direto
    int fase_lunar_id,       // 1=Nova, 2=Crescente, 3=Cheia, 4=Minguante
    AspectMatrix *aspecto_matriz,
    PontosHylegiacos pontos,
    PlotObject *plots       // 0=Sol, 1=Lua, 2=Merc, 3=Ven, 4=Mar, 5=Jup, 6=Sat
);
#endif
