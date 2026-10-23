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

#ifndef NUMBER_HELPER_H
#define NUMBER_HELPER_H

typedef struct {
    int grau;
    int min;
    int sec;
} Grau;

typedef struct {
    int hora;
    int min;
    int sec;
} Hora;

void formatar_tempo_sideral(double ramc, char *out_str);
void formatar_dms(double valor_decimal, char *out_str);
double get_decimal_degrees(Grau c);
void get_degree_str(double grauDecimal, char* resultado);
Hora get_fmt_hour(double h);
int mod_a(int n, int div);
char* intToRoman(int num);
int romanToInt(char *s);

#endif
