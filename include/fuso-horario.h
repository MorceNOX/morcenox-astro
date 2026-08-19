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

#ifndef FUSO_HORARIO_H
#define FUSO_HORARIO_H

int obter_segundos_gmt_na_data(const char* tz_iana, int ano, int mes, int dia, int hora, int minuto);
int obter_segundos_dst_na_data(const char* tz_iana, int ano, int mes, int dia, int hora, int minuto);
int get_gmt_offset(char *city, char *state, char *country);
//void get_iana_timezone(char *city, char *state, char *country, char **timezone);

#endif
