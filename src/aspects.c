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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ncursesw/curses.h>
#include "var.h"
#include "helper.h"
#include "draw-chart.h"
#include "planet_table.h"
#include "db-utils.h"
#include "aspects.h"

// ids começam por 0
bool has_aspect(int id1, int id2, AspectMatrix *matrix) {
    if (id1 < 0 || id2 < 0 || id1 == id2) {
        return false;
    }
    
    AspectCell c1 = matrix->grid[id1][id2];
    AspectCell c2 = matrix->grid[id2][id1];

    if (c1.has_aspect || c2.has_aspect) {
        return true;
    }

    return false;
}


bool has_aspect_aplicative(int id1, int id2, AspectMatrix *matrix) {
    if (id1 < 0 || id2 < 0 || id1 == id2) {
        return false;
    }
    
    AspectCell c1 = matrix->grid[id1][id2];
    AspectCell c2 = matrix->grid[id2][id1];

    if (c1.has_aspect || c2.has_aspect) {
        if (c1.is_aplicative || c2.is_aplicative) {
            return true;
        }
    }

    return false;
}


bool has_aspect_partil(int id1, int id2, AspectMatrix *matrix) {
    if (id1 < 0 || id2 < 0 || id1 == id2) {
        return false;
    }
    
    AspectCell c1 = matrix->grid[id1][id2];
    AspectCell c2 = matrix->grid[id2][id1];

    if (c1.has_aspect || c2.has_aspect) {
        if (c1.is_partil || c2.is_partil) {
            return true;
        }
    }

    return false;
}


bool has_aspect_aplicative_or_partil(int id1, int id2, AspectMatrix *matrix) {
    if (id1 < 0 || id2 < 0 || id1 == id2) {
        return false;
    }
    
    AspectCell c1 = matrix->grid[id1][id2];
    AspectCell c2 = matrix->grid[id2][id1];

    if (c1.has_aspect || c2.has_aspect) {
        if (c1.is_aplicative || c2.is_aplicative || c1.is_partil || c2.is_partil) {
            return true;
        }
    }

    return false;
}


bool has_aspect_separative(int id1, int id2, AspectMatrix *matrix) {
    if (id1 < 0 || id2 < 0 || id1 == id2) {
        return false;
    }
    
    AspectCell c1 = matrix->grid[id1][id2];
    AspectCell c2 = matrix->grid[id2][id1];

    if (c1.has_aspect || c2.has_aspect) {
        if (!has_aspect_aplicative_or_partil(id1, id2, matrix)) {
            return true;
        }
    }

    return false;
}

bool is_under_siege(int planet_id, AspectMatrix *matrix) {
    if ((has_aspect_aplicative(planet_id, P_MARS, matrix) && has_aspect_separative(planet_id, P_SATURN, matrix)) ||
        (has_aspect_aplicative(planet_id, P_SATURN, matrix) && has_aspect_separative(planet_id, P_MARS, matrix))) {
        
        return true;
    }

    return false;
}


bool is_under_assistance(int planet_id, AspectMatrix *matrix) {
    if ((has_aspect_aplicative(planet_id, P_VENUS, matrix) && has_aspect_separative(planet_id, P_JUPITER, matrix)) ||
        (has_aspect_aplicative(planet_id, P_JUPITER, matrix) && has_aspect_separative(planet_id, P_VENUS, matrix))) {
        
        return true;
    }

    return false;
}



AspectMatrix calculate_aspects(PlotObject *plots, double *planet_orbis, PlanetDignities *dig, int *feral, int *vazio_de_curso, int *retro) {
   
    int object_diff = show_modern_planets ? 0 : 3;
    AspectMatrix matrix = {0};

    AspectDefs aspects_defs[] = {
        {0.0, "☌", "Conjunction"}, 
        {60.0, "⚹", "Sextile"}, 
        {90.0, "□", "Square"}, 
        {120.0, "△", "Trine"}, 
        {180.0, "☍", "Opposition"}
    };
    

    for (int i = 0; i < 12 - object_diff; i++) {
        feral[i] = 1;
        vazio_de_curso[i] = 1;
    }

    // Varredura para calcular os aspectos
    for (int i = 0; i < 14 - object_diff; i++) {
        
        for (int j = 0; j < NUM_OBJECTS - object_diff; j++) {
            if (i >= j) continue;

            // returns the angle between -180 and 180 degrees
            double angle = normalize_angle(plots[i].longitude - plots[j].longitude);

            int sinal = (angle > 0) ? 1 : 0;
            angle = fabs(angle);

            // Acha o aspecto mais próximo
            double min_diff = 360.0;
            int closest = -1;
            for (int a = 0; a < 5; a++) {
                double diff = fabs(angle - aspects_defs[a].angle);
                if (diff > 180.0) diff = 360.0 - diff;
                if (diff < min_diff) { min_diff = diff; closest = a; }
            }


            // Verificar combustão, cazimi e sob raios

            if (j > 0 && j < 12 - object_diff) {
                if (plots[i].id == 0 && angle <= 8.5) {
                    if (angle <=  17.0 / 60.0) {
                        dig[j].accidental += 5;
                        dig[j].row.cazimi = 1;
                        dig[j].row.combust = 0;
                        dig[j].row.under_rays = 0;
                    }
                    else {
                        dig[j].accidental -= 6;
                        dig[j].row.cazimi = 0;
                        dig[j].row.combust = 1;
                        dig[j].row.under_rays = 0;
                    }
                } 
                else if (plots[i].id == 0 && angle <= 17) {
                    dig[j].accidental -= 4;
                    dig[j].row.cazimi = 0;
                    dig[j].row.combust = 0;
                    dig[j].row.under_rays = 1;
                } 
                else if (plots[i].id == 0 && angle > 17) {
                    dig[j].accidental += 5;
                    dig[j].row.cazimi = 0;
                    dig[j].row.combust = 0;
                    dig[j].row.under_rays = -1;
                }
            } 


            // Validação de Orbe e Signo
            if (closest >= 0 && min_diff <= (planet_orbis[i] + planet_orbis[j]) / 2.0) {
                int sign_diff = diff_sign((int)floor(plots[i].longitude / 30), (int)floor(plots[j].longitude / 30));
                
                int aspect_diff = fabs(aspects_defs[closest].angle - angle);

                if ((strcmp(aspects_defs[closest].name, "Square") == 0 && sign_diff == 3) ||
                    (strcmp(aspects_defs[closest].name, "Trine") == 0 && sign_diff == 4) ||
                    (strcmp(aspects_defs[closest].name, "Sextile") == 0 && sign_diff == 2) ||
                    (strcmp(aspects_defs[closest].name, "Opposition") == 0 && sign_diff == 6) ||
                    (strcmp(aspects_defs[closest].name, "Conjunction") == 0) ||
                     aspect_diff <= 1) 
                {
                    
                    // se tem aspecto não é feral
                    if (i < 12 - object_diff) {
                            feral[i] = 0;
                            feral[j] = 0;                                
                    } 
                    // Alimenta a célula correspondente da matriz
                    matrix.grid[i][j].has_aspect = true;
                    matrix.grid[i][j].angle = angle;
                    strncpy(matrix.grid[i][j].symbol, aspects_defs[closest].symbol, 3);

                    bool partil = false;
                    if (aspect_diff < 1.0) {
                        partil = true;
                    }
                    
                    int aplicativo = 0;
                    if (!partil && fabs(plots[i].speed) > fabs(plots[j].speed)) {
                        if ((sinal && aspects_defs[closest].angle > angle) || (!sinal && aspects_defs[closest].angle < angle)) {
                            if (retro[i]) {
                                aplicativo = 0;
                            }
                            else {
                                aplicativo = 1;
                                vazio_de_curso[i] = 0;
                            }                                
                        }
                        else if ((sinal && aspects_defs[closest].angle < angle) || (!sinal && aspects_defs[closest].angle > angle)) {
                            if (retro[i]) {
                                aplicativo = 1;
                                vazio_de_curso[i] = 0;
                            }
                            else {
                                aplicativo = 0;
                            }                                
                        }
                    }
                    else if (!partil) {
                        if ((sinal && aspects_defs[closest].angle < angle) || (!sinal && aspects_defs[closest].angle > angle)) {
                            if (retro[j]) {
                                aplicativo = 0;
                            }
                            else {
                                aplicativo = 1;
                                vazio_de_curso[j] = 0;
                            }                                
                        }
                        else if ((sinal && aspects_defs[closest].angle > angle) || (!sinal && aspects_defs[closest].angle < angle)) {
                            if (retro[j]) {
                                aplicativo = 1;
                                vazio_de_curso[j] = 0;
                            }
                            else {
                                aplicativo = 0;
                            }                                
                        }
                    }

                    if (i < 12 - object_diff) {
                        if (aplicativo || partil) {
                            vazio_de_curso[i] = 0;
                            vazio_de_curso[j] = 0;
                        }                            
                    }

                    if (partil) {
                        matrix.grid[i][j].is_partil = true;
                    }
                    else {
                        matrix.grid[i][j].is_partil = false;
                    }
                    
                    if (aplicativo && !partil) {
                        strncat(matrix.grid[i][j].symbol, " a", 3);
                        matrix.grid[i][j].is_aplicative = true;
                    }
                    else if (!partil) {
                        strncat(matrix.grid[i][j].symbol, " s", 3);
                        matrix.grid[i][j].is_aplicative = false;
                    }
                    
                    // Injeta a lógica de cores e estilos diretamente no dado
                    if (strcmp(aspects_defs[closest].name, "Square") == 0 || strcmp(aspects_defs[closest].name, "Opposition") == 0) {
                        matrix.grid[i][j].color_pair = 11;
                        matrix.grid[i][j].is_bold = true;
                    } else if (strcmp(aspects_defs[closest].name, "Trine") == 0 || strcmp(aspects_defs[closest].name, "Sextile") == 0) {
                        matrix.grid[i][j].color_pair = 8;
                        matrix.grid[i][j].is_bold = false;
                    } else {
                        matrix.grid[i][j].color_pair = 7;
                        matrix.grid[i][j].is_bold = true;
                    }
                }
            }
        }

    }

    return matrix;
}


DeclMatrix calculate_declination_aspects(PlotObject *plots, double decl_orbis) {
    int object_diff = show_modern_planets ? 0 : 3;
    DeclMatrix matrix_decl = {0};

    for (int i = 0; i < 14 - object_diff; i++) {
        for (int j = 0; j < NUM_OBJECTS - object_diff; j++) {
            if (i >= j) continue;

            double dec_i = plots[i].declination;
            double dec_j = plots[j].declination;

            double diff_parallel = fabs(dec_i - dec_j);
            double diff_contra = fabs(fabs(dec_i) - fabs(dec_j));
            bool mesmo_sinal = ((dec_i >= 0 && dec_j >= 0) || (dec_i < 0 && dec_j < 0));

            if (diff_parallel <= decl_orbis && mesmo_sinal) {
                matrix_decl.grid[i][j].has_aspect = true;
                strncpy(matrix_decl.grid[i][j].symbol, "∥", 4);
                matrix_decl.grid[i][j].diff = diff_parallel;
                matrix_decl.grid[i][j].color_pair = 8; // Verde para Paralelo
            } 
            else if (diff_contra <= decl_orbis && !mesmo_sinal) {
                matrix_decl.grid[i][j].has_aspect = true;
                strncpy(matrix_decl.grid[i][j].symbol, "∦", 4);
                matrix_decl.grid[i][j].diff = diff_contra;
                matrix_decl.grid[i][j].color_pair = 11; // Vermelho para Contra-Paralelo
            }
        }            
    }

    return matrix_decl;

}




void display_declination_aspects(PlotObject *plots, DeclMatrix *matrix) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int table_height = 24;
    int table_width = max_x - 5;
    int start_y = (max_y - table_height) / 2;
    int start_x = 2;

    int object_diff = 0;
    if (show_modern_planets) {
        object_diff = 0;
    }
    else {
        object_diff = 3;
    }

    WINDOW *decl_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *decl_shadow = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    werase(decl_shadow);
    wattron(decl_shadow, COLOR_PAIR(9));
    box(decl_shadow, 0, 0);
    wattroff(decl_shadow, COLOR_PAIR(9));
    wrefresh(decl_shadow);

    box(decl_win, 0, 0);
    wbkgd(decl_win, COLOR_PAIR(6));

    wattron(decl_win, A_BOLD);
    const char *title = _("Parallel & Contra-Parallel");
    mvwprintw(decl_win, 0, (table_width - get_visual_width(title)) / 2, title);
    
    int max_linhas_dados = table_height - 6;
    WINDOW *pad = newpad(40, table_width - 4);
    wbkgd(pad, COLOR_PAIR(13));

    int row_pad = 0;
    
    // 1. Cabeçalhos Superiores (Símbolos dos Planetas)
    for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
        wattron(decl_win, A_BOLD);
        mvwprintw(decl_win, 1, 6 + 4 * i, plots[i].object);
        wattroff(decl_win, A_BOLD);
    }

    // 2. Cabeçalhos Laterais (Símbolos dos Planetas)
    for (int i = 0; i < 12 - object_diff; i++) {
        wattron(pad, A_BOLD);
        mvwprintw(pad, 1 + 2 * i, 0, plots[i].object);
        wattroff(pad, A_BOLD);
    }

    // 3. Desenho das Linhas do Grid
    for (int i = 0; i < 12 - object_diff + 1; i++) {
        for (int j = 0; j < NUM_OBJECTS - object_diff; j++) {
            if (i == 0) {
                mvwprintw(decl_win, 2, 4 + 4 * j, "_____");
            }
            else {
                mvwprintw(pad, 0 + 2 * i, 2 + 4 * j, "_____");
            }
            
        }
    }

    // 4. Desenho das Linhas Verticais
    for (int i = 0; i < ((12 - object_diff) * 2); i++) {
        for (int j = 0; j < NUM_OBJECTS - object_diff + 1; j++) {
            mvwprintw(pad, 1 + i, 2 + 4 * j, "|");
        }
    }

    // 5. Renderização dos Dados Pré-Calculados da Matriz
    for (int i = 0; i < 12 - object_diff; i++) {
        for (int j = 0; j < NUM_OBJECTS - object_diff; j++) {
            
            // Se i >= j, renderiza o bloco nulo/vazio (Triângulo inferior da matriz)
            if (i >= j) {
                wattron(pad, COLOR_PAIR(10) | A_DIM);
                mvwprintw(pad, 1 + 2 * i, 3 + 4 * j, "▓▓▓");
                wattroff(pad, COLOR_PAIR(10) | A_DIM);
                continue;
            }


            DeclCell cell = matrix->grid[i][j];

            if (cell.has_aspect) {
                // Exibe o símbolo do aspecto injetado
                wattron(pad, COLOR_PAIR(cell.color_pair) | A_BOLD);
                mvwprintw(pad, 1 + 2 * i, 5 + 4 * j, cell.symbol);
                wattroff(pad, COLOR_PAIR(cell.color_pair) | A_BOLD);
                
                // Exibe os minutos/graus residuais exatos do orbe formatado
                wattron(pad, COLOR_PAIR(10) | A_DIM);
                char orbe_str[8];
                snprintf(orbe_str, 8, "%3.1f", cell.diff);
                mvwprintw(pad, 2 + 2 * i, 3 + 4 * j, orbe_str);
                wattroff(pad, COLOR_PAIR(10) | A_DIM);

                row_pad = 2 + 2 * i + 1;
            }
        }
    }

    mvwprintw(decl_win, table_height - 1, 2, _("Press ESC to return - [↓↑|JK] Scroll"));
    wrefresh(decl_win);

    int offset_y = 0;
    int max_scroll_y = row_pad - max_linhas_dados + 2;
    if (max_scroll_y < 0) max_scroll_y = 0;

    // Vincula o teclado à PAD virtual
    keypad(pad, TRUE);
    nodelay(pad, FALSE);

    // Renderiza a primeira foto da PAD na tela
    prefresh(pad, offset_y + 1, 0, start_y + 3, start_x + 2, start_y + table_height - 2, start_x + table_width - 3);

    int ch;
    while ((ch = wgetch(pad)) != 27 && ch != 'q' && ch != 'Q') {        
        switch (ch) {
            case KEY_UP: 
            case 'k': 
            case 'K':
                if (offset_y > 0) offset_y -= 2;
                break;
                
            case KEY_DOWN: 
            case 'j': 
            case 'J':
                if (offset_y < max_scroll_y) offset_y += 2;
                break;
        }
        prefresh(pad, offset_y + 1, 0, start_y + 3, start_x + 2, start_y + table_height - 2, start_x + table_width - 3);        
    }
    
    // CLEAN UP: Desaloca todas as janelas do escopo e devolve o controle para a stdscr limpa
    delwin(pad);
    delwin(decl_shadow);
    delwin(decl_win);
    refresh();
}



void display_aspects(PlotObject *plots, AspectMatrix *matrix, DeclMatrix *matrix_decl, AntObject *ants, int num_ants) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int table_height = 24;
    int table_width = max_x - 5;
    int start_y = (max_y - table_height) / 2;
    int start_x = 2;

    int object_diff = 0;
    if (show_modern_planets) {
        object_diff = 0;
    }
    else {
        object_diff = 3;
    }

    WINDOW *aspects_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *aspects_shadow = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    werase(aspects_shadow);
    wattron(aspects_shadow, COLOR_PAIR(9));
    box(aspects_shadow, 0, 0);
    wattroff(aspects_shadow, COLOR_PAIR(9));
    wrefresh(aspects_shadow);

    box(aspects_win, 0, 0);
    wbkgd(aspects_win, COLOR_PAIR(6));
    wattron(aspects_win, A_BOLD);
    const char *title = _("Aspects Table");
    mvwprintw(aspects_win, 0, (table_width - get_visual_width(title)) / 2, title);



    int max_linhas_dados = table_height - 6;
    WINDOW *pad = newpad(40, table_width - 4);
    wbkgd(pad, COLOR_PAIR(13));

    int row_pad = 0;
    
    // 1. Cabeçalhos Superiores (Símbolos dos Planetas)
    for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
        wattron(aspects_win, A_BOLD);
        mvwprintw(aspects_win, 1, 6 + 4 * i, plots[i].object);
        wattroff(aspects_win, A_BOLD);
    }

    // 2. Cabeçalhos Laterais (Símbolos dos Planetas)
    for (int i = 0; i < 12 - object_diff; i++) {
        wattron(pad, A_BOLD);
        mvwprintw(pad, 1 + 2 * i, 0, plots[i].object);
        wattroff(pad, A_BOLD);
    }

    // 3. Desenho das Linhas do Grid
    for (int i = 0; i < 12 - object_diff + 1; i++) {
        for (int j = 0; j < NUM_OBJECTS - object_diff; j++) {
            if (i == 0) {
                mvwprintw(aspects_win, 2, 4 + 4 * j, "_____");
            }
            else {
                mvwprintw(pad, 0 + 2 * i, 2 + 4 * j, "_____");
            }
            
        }
    }

    // 4. Desenho das Linhas Verticais
    for (int i = 0; i < ((12 - object_diff) * 2); i++) {
        for (int j = 0; j < NUM_OBJECTS - object_diff + 1; j++) {
            mvwprintw(pad, 1 + i, 2 + 4 * j, "|");
        }
    }

    // 5. Renderização dos Dados Pré-Calculados da Matriz
    for (int i = 0; i < 12 - object_diff; i++) {
        for (int j = 0; j < NUM_OBJECTS - object_diff; j++) {
            
            // Se i >= j, renderiza o bloco nulo/vazio (Triângulo inferior da matriz)
            if (i >= j) {
                wattron(pad, COLOR_PAIR(10) | A_DIM);
                mvwprintw(pad, 1 + 2 * i, 3 + 4 * j, "▓▓▓");
                wattroff(pad, COLOR_PAIR(10) | A_DIM);
                continue;
            }

            // Pega a célula correspondente
            AspectCell cell = matrix->grid[i][j];

            if (cell.has_aspect) {
                // Ativa os atributos dinâmicos injetados pela função chamadora
                wattron(pad, COLOR_PAIR(cell.color_pair));
                if (cell.is_bold) wattron(pad, A_BOLD);

                // Desenha o Símbolo Astrológico do Aspecto
                mvwprintw(pad, 1 + 2 * i, 3 + 4 * j, cell.symbol);
                
                // Desativa os atributos do símbolo
                if (cell.is_bold) wattroff(pad, A_BOLD);
                wattroff(pad, COLOR_PAIR(cell.color_pair));
                
                // Desenha o valor numérico do ângulo abaixo do símbolo
                wattron(pad, COLOR_PAIR(10) | A_DIM);
                char ag[8];
                snprintf(ag, 8, "%3.0f", cell.angle);
                mvwprintw(pad, 2 + 2 * i, 3 + 4 * j, ag);
                wattroff(pad, COLOR_PAIR(10) | A_DIM);

                row_pad = 2 + 2 * i + 1;
            }
            // else {
            //     wattron(pad, COLOR_PAIR(10) | A_DIM);
            //     mvwprintw(pad, 1 + 2 * i, 3 + 4 * j, "░░░"); 
            //     wattroff(pad, COLOR_PAIR(10) | A_DIM);
            // }
        }
    }

    mvwprintw(aspects_win, table_height - 1, 2, _("Press ESC to return to chart - F3 Parallel & Contra-parallel - F4 Aspects by Sign - F5 Antissia - [↓↑|JK] Scroll"));
    wrefresh(aspects_win);

    int offset_y = 0;
    int max_scroll_y = row_pad - max_linhas_dados + 2;
    if (max_scroll_y < 0) max_scroll_y = 0;

    // Vincula o teclado à PAD virtual
    keypad(pad, TRUE);
    nodelay(pad, FALSE);

    // Renderiza a primeira foto da PAD na tela
    prefresh(pad, offset_y + 1, 0, start_y + 3, start_x + 2, start_y + table_height - 2, start_x + table_width - 3);

    int ch;
    while ((ch = wgetch(pad)) != 27 && ch != 'q' && ch != 'Q') {
        
        if (ch == KEY_F(3)) {
            display_declination_aspects(plots, matrix_decl);
            
            touchwin(aspects_shadow); // Marca a janela de sombra para atualização total
            wrefresh(aspects_shadow);
            touchwin(aspects_win);  // Marca a janela da tabela para atualização total
            wrefresh(aspects_win);
            prefresh(pad, offset_y + 1, 0, start_y + 3, start_x + 2, start_y + table_height - 2, start_x + table_width - 3);
        }
        else if (ch == KEY_F(4)) {
            AspectMatrix matrix_sign = {0}; 
            matrix_sign = calculate_aspects_by_sign(plots);

            display_aspects_by_sign(plots, &matrix_sign);
            
            touchwin(aspects_shadow);
            wrefresh(aspects_shadow);
            touchwin(aspects_win);
            wrefresh(aspects_win);
            prefresh(pad, offset_y + 1, 0, start_y + 3, start_x + 2, start_y + table_height - 2, start_x + table_width - 3);
        }
        else if (ch == KEY_F(5)) {
            AspectMatrix matrix_ants = {0}; 
            matrix_ants = calculate_aspects_antiscium(plots, ants, num_ants);

            display_aspects_antissium(plots, ants, num_ants, &matrix_ants);
            
            touchwin(aspects_shadow);
            wrefresh(aspects_shadow);
            touchwin(aspects_win);
            wrefresh(aspects_win);
            prefresh(pad, offset_y + 1, 0, start_y + 3, start_x + 2, start_y + table_height - 2, start_x + table_width - 3);
        }
        else {
            // Se não foi nenhuma tecla de função, processa a rolagem vertical do texto
            switch (ch) {
                case KEY_UP: 
                case 'k': 
                case 'K':
                    if (offset_y > 0) offset_y -= 2;
                    break;
                    
                case KEY_DOWN: 
                case 'j': 
                case 'J':
                    if (offset_y < max_scroll_y) offset_y += 2;
                    break;
            }
            // Atualiza os frames da PAD na tela após o movimento de subida/descida
            prefresh(pad, offset_y + 1, 0, start_y + 3, start_x + 2, start_y + table_height - 2, start_x + table_width - 3);
        }
    }
    
    // CLEAN UP: Desaloca todas as janelas do escopo e devolve o controle para a stdscr limpa
    delwin(pad);
    delwin(aspects_shadow);
    delwin(aspects_win);
    touchwin(stdscr); 
    refresh();
}



AspectMatrix calculate_aspects_by_sign(PlotObject *plots) {
   
    int object_diff = show_modern_planets ? 0 : 3;
    AspectMatrix matrix = {0};

    AspectDefs aspects_defs[] = {
        {0.0, "☌", "Conjunction"}, 
        {2.0, "⚹", "Sextile"}, 
        {3.0, "□", "Square"}, 
        {4.0, "△", "Trine"}, 
        {6.0, "☍", "Opposition"}
    };
    

    
    for (int i = 0; i < 14 - object_diff; i++) {
        
        for (int j = 0; j < NUM_OBJECTS - object_diff; j++) {
            if (i >= j) continue;

            int sign_diff = diff_sign((int)floor(plots[i].longitude / 30), (int)floor(plots[j].longitude / 30));
                     
            if (sign_diff == 3 ||
                sign_diff == 4 ||
                sign_diff == 2 ||
                sign_diff == 6 ||
                sign_diff == 0) 
            {

                int aspect = -1;
                for (int a = 0; a < 5; a++) {
                    if ((double)sign_diff == aspects_defs[a].angle) {
                        aspect = a;
                    }
                }
                                     
                matrix.grid[i][j].has_aspect = true;
                matrix.grid[i][j].angle = (double)sign_diff;
                strncpy(matrix.grid[i][j].symbol, aspects_defs[aspect].symbol, 3);
                                      
                // Injeta a lógica de cores e estilos diretamente no dado
                if (strcmp(aspects_defs[aspect].name, "Square") == 0 || strcmp(aspects_defs[aspect].name, "Opposition") == 0) {
                    matrix.grid[i][j].color_pair = 11;
                    matrix.grid[i][j].is_bold = true;
                } else if (strcmp(aspects_defs[aspect].name, "Trine") == 0 || strcmp(aspects_defs[aspect].name, "Sextile") == 0) {
                    matrix.grid[i][j].color_pair = 8;
                    matrix.grid[i][j].is_bold = false;
                } else {
                    matrix.grid[i][j].color_pair = 7;
                    matrix.grid[i][j].is_bold = true;
                }
            }
            
        }

    }

    return matrix;
}



void display_aspects_by_sign(PlotObject *plots, AspectMatrix *matrix) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int table_height = 24;
    int table_width = max_x - 5;
    int start_y = (max_y - table_height) / 2;
    int start_x = 2;

    int object_diff = 0;
    if (show_modern_planets) {
        object_diff = 0;
    }
    else {
        object_diff = 3;
    }

    WINDOW *aspects_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *aspects_shadow = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    werase(aspects_shadow);
    wattron(aspects_shadow, COLOR_PAIR(9));
    box(aspects_shadow, 0, 0);
    wattroff(aspects_shadow, COLOR_PAIR(9));
    wrefresh(aspects_shadow);

    box(aspects_win, 0, 0);
    wbkgd(aspects_win, COLOR_PAIR(6));
    wattron(aspects_win, A_BOLD);
    const char *title = _("Aspects by Sign Table");
    mvwprintw(aspects_win, 0, (table_width - get_visual_width(title)) / 2, title);



    int max_linhas_dados = table_height - 6;
    WINDOW *pad = newpad(40, table_width - 4);
    wbkgd(pad, COLOR_PAIR(13));

    int row_pad = 0;
    
    // 1. Cabeçalhos Superiores (Símbolos dos Planetas)
    for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
        wattron(aspects_win, A_BOLD);
        mvwprintw(aspects_win, 1, 6 + 4 * i, plots[i].object);
        wattroff(aspects_win, A_BOLD);
    }

    // 2. Cabeçalhos Laterais (Símbolos dos Planetas)
    for (int i = 0; i < 12 - object_diff; i++) {
        wattron(pad, A_BOLD);
        mvwprintw(pad, 1 + 2 * i, 0, plots[i].object);
        wattroff(pad, A_BOLD);
    }

    // 3. Desenho das Linhas do Grid
    for (int i = 0; i < 12 - object_diff + 1; i++) {
        for (int j = 0; j < NUM_OBJECTS - object_diff; j++) {
            if (i == 0) {
                mvwprintw(aspects_win, 2, 4 + 4 * j, "_____");
            }
            else {
                mvwprintw(pad, 0 + 2 * i, 2 + 4 * j, "_____");
            }
            
        }
    }

    // 4. Desenho das Linhas Verticais
    for (int i = 0; i < ((12 - object_diff) * 2); i++) {
        for (int j = 0; j < NUM_OBJECTS - object_diff + 1; j++) {
            mvwprintw(pad, 1 + i, 2 + 4 * j, "|");
        }
    }

    // 5. Renderização dos Dados Pré-Calculados da Matriz
    for (int i = 0; i < 12 - object_diff; i++) {
        for (int j = 0; j < NUM_OBJECTS - object_diff; j++) {
            
            // Se i >= j, renderiza o bloco nulo/vazio (Triângulo inferior da matriz)
            if (i >= j) {
                wattron(pad, COLOR_PAIR(10) | A_DIM);
                mvwprintw(pad, 1 + 2 * i, 3 + 4 * j, "▓▓▓");
                wattroff(pad, COLOR_PAIR(10) | A_DIM);
                continue;
            }

            // Pega a célula correspondente
            AspectCell cell = matrix->grid[i][j];

            if (cell.has_aspect) {
                // Ativa os atributos dinâmicos injetados pela função chamadora
                wattron(pad, COLOR_PAIR(cell.color_pair));
                
                if (cell.is_bold) wattron(pad, A_BOLD);

                // Desenha o Símbolo Astrológico do Aspecto
                mvwprintw(pad, 1 + 2 * i, 3 + 4 * j, cell.symbol);
                
                // Desativa os atributos do símbolo
                if (cell.is_bold) wattroff(pad, A_BOLD);
                wattroff(pad, COLOR_PAIR(cell.color_pair));
                                
                row_pad = 2 + 2 * i + 1;
            }
            else {
                wattron(pad, COLOR_PAIR(10) | A_DIM);
                mvwprintw(pad, 1 + 2 * i, 3 + 4 * j, "░░░"); 
                wattroff(pad, COLOR_PAIR(10) | A_DIM);
            }
        }
    }

    mvwprintw(aspects_win, table_height - 1, 2, _("Press ESC to return - [↓↑|JK] Scroll"));
    wrefresh(aspects_win);

    int offset_y = 0;
    int max_scroll_y = row_pad - max_linhas_dados + 2;
    if (max_scroll_y < 0) max_scroll_y = 0;

    // Vincula o teclado à PAD virtual
    keypad(pad, TRUE);
    nodelay(pad, FALSE);

    // Renderiza a primeira foto da PAD na tela
    prefresh(pad, offset_y + 1, 0, start_y + 3, start_x + 2, start_y + table_height - 2, start_x + table_width - 3);

    int ch;
    while ((ch = wgetch(pad)) != 27 && ch != 'q' && ch != 'Q') {
                
        switch (ch) {
            case KEY_UP: 
            case 'k': 
            case 'K':
                if (offset_y > 0) offset_y -= 2;
                break;
                
            case KEY_DOWN: 
            case 'j': 
            case 'J':
                if (offset_y < max_scroll_y) offset_y += 2;
                break;
        }
        prefresh(pad, offset_y + 1, 0, start_y + 3, start_x + 2, start_y + table_height - 2, start_x + table_width - 3);
        
    }
    
    delwin(pad);
    delwin(aspects_shadow);
    delwin(aspects_win);
    touchwin(stdscr); 
    refresh();
}



AspectMatrix calculate_aspects_antiscium(PlotObject *plots, AntObject *ants, int num_objects) {
   
    AspectMatrix matrix = {0};

    AspectDefs aspects_defs[] = {
        {0.0, "☌", "Conjunction"}, 
        {60.0, "⚹", "Sextile"}, 
        {90.0, "□", "Square"}, 
        {120.0, "△", "Trine"}, 
        {180.0, "☍", "Opposition"}
    };
    
    // Varredura para calcular os aspectos
    for (int i = 0; i < 7; i++) {
        
        for (int j = 0; j < num_objects; j++) {
            if (i >= j) continue;

            // returns the angle between -180 and 180 degrees
            double angle = normalize_angle(plots[i].longitude - ants[j].longitude);

            angle = fabs(angle);

            // Acha o aspecto mais próximo
            double min_diff = 360.0;
            int closest = -1;
            for (int a = 0; a < 5; a++) {
                double diff = fabs(angle - aspects_defs[a].angle);
                if (diff > 180.0) diff = 360.0 - diff;
                if (diff < min_diff) { min_diff = diff; closest = a; }
            }

            // Validação de Orbe e Signo
            if (closest >= 0 && min_diff <= ANTISCIUM_ORB) {
               
                double aspect_diff = fabs(aspects_defs[closest].angle - angle);

                 
                // Alimenta a célula correspondente da matriz
                matrix.grid[i][j].has_aspect = true;
                matrix.grid[i][j].angle = aspect_diff;
                strncpy(matrix.grid[i][j].symbol, aspects_defs[closest].symbol, 3);

                // Injeta a lógica de cores e estilos diretamente no dado
                if (strcmp(aspects_defs[closest].name, "Square") == 0 || strcmp(aspects_defs[closest].name, "Opposition") == 0) {
                    matrix.grid[i][j].color_pair = 11;
                    matrix.grid[i][j].is_bold = true;
                } else if (strcmp(aspects_defs[closest].name, "Trine") == 0 || strcmp(aspects_defs[closest].name, "Sextile") == 0) {
                    matrix.grid[i][j].color_pair = 8;
                    matrix.grid[i][j].is_bold = false;
                } else {
                    matrix.grid[i][j].color_pair = 7;
                    matrix.grid[i][j].is_bold = true;
                }
                
            }
        }

    }

    return matrix;
}




void display_aspects_antissium(PlotObject *plots, AntObject *ants, int num_ants, AspectMatrix *matrix) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int table_height = 24;
    int table_width = max_x - 5;
    int start_y = (max_y - table_height) / 2;
    int start_x = 2;

    WINDOW *aspects_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *aspects_shadow = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    werase(aspects_shadow);
    wattron(aspects_shadow, COLOR_PAIR(9));
    box(aspects_shadow, 0, 0);
    wattroff(aspects_shadow, COLOR_PAIR(9));
    wrefresh(aspects_shadow);

    box(aspects_win, 0, 0);
    wbkgd(aspects_win, COLOR_PAIR(6));
    wattron(aspects_win, A_BOLD);
    const char *title = _(" Antissia & Contrantissia Aspect Matrix Grid ");
    mvwprintw(aspects_win, 0, (table_width - get_visual_width(title)) / 2, title);



    int max_linhas_dados = table_height - 6;
    WINDOW *pad = newpad(40, table_width - 4);
    wbkgd(pad, COLOR_PAIR(13));

    int row_pad = 0;
    
    // 1. Cabeçalhos Superiores (Símbolos dos Planetas)
    for (int i = 0; i < num_ants; i++) {

        int degree = (int)fmod(ants[i].longitude, 30);
        const char *sign_str = get_sign((int)(ants[i].longitude / 30));
        char text[10];
        snprintf(text, 10, "%d%s", degree, sign_str); 

        wattron(aspects_win, A_BOLD);
        mvwprintw(aspects_win, 1, 6 + 6 * i, ants[i].object);
        wattroff(aspects_win, A_BOLD);
        mvwprintw(aspects_win, 2, 6 + 6 * i, text);
    }

    // 2. Cabeçalhos Laterais (Símbolos dos Planetas)
    for (int i = 0; i < 7; i++) {
        wattron(pad, A_BOLD);
        mvwprintw(pad, 2 + 2 * i, 0, plots[i].object);
        wattroff(pad, A_BOLD);
    }

    // 3. Desenho das Linhas do Grid
    for (int i = 0; i < 7 + 1; i++) {
        for (int j = 0; j < num_ants; j++) {
            if (i == 0) {
                mvwprintw(aspects_win, 3, 4 + 6 * j, "______");
            }
            else {
                mvwprintw(pad, 1 + 2 * i, 2 + 6 * j, "______");
            }
            
        }
    }

    // 4. Desenho das Linhas Verticais
    for (int i = 0; i < (7 * 2); i++) {
        for (int j = 0; j < num_ants + 1; j++) {
            mvwprintw(pad, 2 + i, 2 + 6 * j, "|");
        }
    }

    // 5. Renderização dos Dados Pré-Calculados da Matriz
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < num_ants; j++) {
            
            // Se i >= j, renderiza o bloco nulo/vazio (Triângulo inferior da matriz)
            if (i >= j) {
                wattron(pad, COLOR_PAIR(10) | A_DIM);
                mvwprintw(pad, 2 + 2 * i, 3 + 6 * j, "▓▓▓▓▓");
                wattroff(pad, COLOR_PAIR(10) | A_DIM);
                continue;
            }

            // Pega a célula correspondente
            AspectCell cell = matrix->grid[i][j];

            if (cell.has_aspect) {
                // Ativa os atributos dinâmicos injetados pela função chamadora
                wattron(pad, COLOR_PAIR(cell.color_pair));
                
                if (cell.is_bold) wattron(pad, A_BOLD);

                // Desenha o Símbolo Astrológico do Aspecto
                mvwprintw(pad, 2 + 2 * i, 3 + 6 * j, cell.symbol);
                
                // Desativa os atributos do símbolo
                if (cell.is_bold) wattroff(pad, A_BOLD);
                wattroff(pad, COLOR_PAIR(cell.color_pair));

                wattron(pad, COLOR_PAIR(10) | A_DIM);
                char ag[8] = "";
                snprintf(ag, 8, "%4.2f", cell.angle);
                mvwprintw(pad, 3 + 2 * i, 3 + 6 * j, ag);
                wattroff(pad, COLOR_PAIR(10) | A_DIM);
                                
                row_pad = 2 + 2 * i + 1;
            }
            else {
                wattron(pad, COLOR_PAIR(10) | A_DIM);
                mvwprintw(pad, 2 + 2 * i, 3 + 6 * j, "░░░░░"); 
                wattroff(pad, COLOR_PAIR(10) | A_DIM);
            }
        }
    }

    mvwprintw(aspects_win, table_height - 1, 2, _("Press ESC to return - [↓↑|JK] Scroll"));
    wrefresh(aspects_win);

    int offset_y = 0;
    int max_scroll_y = row_pad - max_linhas_dados + 2;
    if (max_scroll_y < 0) max_scroll_y = 0;

    // Vincula o teclado à PAD virtual
    keypad(pad, TRUE);
    nodelay(pad, FALSE);

    // Renderiza a primeira foto da PAD na tela
    prefresh(pad, offset_y + 2, 0, start_y + 4, start_x + 2, start_y + table_height - 2, start_x + table_width - 3);

    int ch;
    while ((ch = wgetch(pad)) != 27 && ch != 'q' && ch != 'Q') {
                
        switch (ch) {
            case KEY_UP: 
            case 'k': 
            case 'K':
                if (offset_y > 0) offset_y -= 2;
                break;
                
            case KEY_DOWN: 
            case 'j': 
            case 'J':
                if (offset_y < max_scroll_y) offset_y += 2;
                break;
        }
        prefresh(pad, offset_y + 2, 0, start_y + 4, start_x + 2, start_y + table_height - 2, start_x + table_width - 3);
        
    }
    
    delwin(pad);
    delwin(aspects_shadow);
    delwin(aspects_win);
    touchwin(stdscr); 
    refresh();
}
