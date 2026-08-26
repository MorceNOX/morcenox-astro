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


#include <ncurses.h>

#ifndef HELPER_H
#define HELPER_H

const char *str_dow(int dow);
int get_int_greater_if_found(int *array, int count, int n, int distance);
int get_int_lesser_if_found(int *array, int count, int n, int distance);
int comparar_plots_por_id(const void *a, const void *b);
int comparar_plots_por_longitude(const void *a, const void *b);
int comparar_directions_por_idade(const void *a, const void *b);
int comparar_doubles(const void *a, const void *b);
int testar_caminho_efemerides();
int inicializar_swiss_ephemeris();
int verificar_arquivos_efemerides();
int get_visual_width(const char *str);
unsigned short get_terminal_width(void);
unsigned short get_terminal_height(void);
void draw_centered_text(WINDOW *win, int y, int x, int available_width, const char *text, int attr);
int find_first_item_with_letter(const char **items, int count, int start_index, char letter);
int find_first_item_with_letter_offset(const char **items, int count, int start_index, char letter, int offset);
int calculate_max_display_items(int menu_height);
void draw_scrolled_menu(WINDOW *win, const char **items, int count, int selected, int start_index, int max_items);
char* load_file_content(const char* filename);
char** split_lines(char* content, int* line_count);
char** split_lines_wrap(char* content, int* line_count, int max_width);
int print_text_multiline(WINDOW *win, int linha_inicial, int coluna_inicial, int max_colunas_linha, const char *texto);
int imprimir_texto_fluxo(WINDOW *win, int coluna_inicial, int max_colunas_linha, const char *texto);
#endif
