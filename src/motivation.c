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

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ncursesw/curses.h>
#include <wchar.h>
#include "var.h"
#include "helper.h"
#include "draw-chart.h"
#include "planet_table.h"
#include "db-utils.h"
#include "hyleg.h"
#include "motivation.h"

const char *get_motivation(int sign) {
    switch (sign) {
        case 0: return _("Immediate action, pioneering, and personal conquest.");
        case 1: return _("Material security, comfort, and sensory stability.");
        case 2: return _("Curiosity, communication, and mental stimulation.");
        case 3: return _("Belonging, emotional protection, and deep roots.");
        case 4: return _("Recognition, creative expression, and validation.");
        case 5: return _("Utility, order, and continuous improvement.");
        case 6: return _("Harmony, relational balance, and social justice.");
        case 7: return _("Depth, psychological transformation, and hidden truths.");
        case 8: return _("Expansion, freedom, and seeking ultimate meaning.");
        case 9: return _("Tangible achievement, status, and building legacy.");
        case 10: return _("Innovation, collective freedom, and social progress.");
        case 11: return _("Mystical union, compassion, and spiritual transcendence.");
    }

    return " ";
}


const char *get_planet_motivation_modifier(int planet_id) {
    switch (planet_id) {
        case 0: return _("The Sun centralizes the motivation in the self, in the quest for personal brilliance, leadership, pride and visibility.");
        case 1: return _("The Moon turns the motivation highly fluctuating, guided by humors, instinct of protection, memory and emotional attachment.");
        case 2: return _("Mercury filters the motivation by logic, intelectualization, necessity of communication, analysis and data processing.");
        case 3: return _("Venus shifts the quest towards the pleasure, aesthetics, partnership, financial gain or social validation.");
        case 4: return _("Mars adds urgency, competitiveness, a need to win, haste, and a focus on direct action.");
        case 5: return _("Jupiter amplifies the motivation with idealism, excesses, quest for wisdom, luck or dogmatism.");
        case 6: return _("Saturn imposes limits, slowness, a sense of duty, a need for structure, fear of failure, and maturity.");
    }

    return " ";
}


const char *get_house_modifier(int house_num) {
    switch(house_num) {
        case 1:  return _("personal identity, vitality, and core temperament");
        case 2:  return _("material wealth, personal finances, and livelihood");
        case 3:  return _("the immediate environment, siblings, and short travels");
        case 4:  return _("ancestry, parental heritage, real estate, and private foundations");
        case 5:  return _("creative expression, children, pleasures, and speculation");
        case 6:  return _("physical illness, daily labor, and subordinate relationships");
        case 7:  return _("partnerships, marriage, public adversaries, and contracts");
        case 8:  return _("inheritances, psychological transformations, and shared resources");
        case 9:  return _("higher knowledge, philosophy, worldview, and long-distance journeys");
        case 10: return _("professional career, social status, and public reputation");
        case 11: return _("alliances, supportive networks, friendships, and long-term hopes");
        case 12: return _("hidden challenges, self-undoing, isolation, and confinement");
    }
    return " ";
}






void display_motivation(PlotObject *plots, int *house_rulers) {

    int object_diff = show_modern_planets ? 0 : 3;

    int term_w, term_h;
    getmaxyx(stdscr, term_h, term_w);
    
    int table_width = term_w - 10;
    int table_height = 29;
    int start_x = (term_w - table_width) / 2;
    int start_y = (term_h - table_height) / 2;

    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    int row_pad = 0;

    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wrefresh(shadow_win);

    werase(table_win);
    box(table_win, 0, 0);
    wbkgd(table_win, COLOR_PAIR(13) | FLAGS); 
    
    wattron(table_win, A_BOLD);
    const char *title = _(" Primary Motivation ");
    mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);
    wattroff(table_win, A_BOLD);

    int max_linhas_dados = table_height - 6;
    WINDOW *pad = newpad(100, table_width - 4);
    wbkgd(pad, COLOR_PAIR(13) | FLAGS);

   
    // Bloco 1: Condição das Faculdades Mentais
    wattron(pad, A_BOLD);
    mvwprintw(pad, 1, 4, _("Sign of Ascendant:"));
    wattroff(pad, A_BOLD);
    
    row_pad++;

    wattron(pad, COLOR_PAIR(10) | A_DIM);
    mvwprintw(pad, 2, 4, "───────────────────────────────────────────────────────────────────────────────────");
    wattroff(pad, COLOR_PAIR(10) | A_DIM);

    row_pad++;

    wattron(pad, COLOR_PAIR(15) | A_BOLD);
    mvwprintw(pad, 3, 4, "%s %s - %s  ", plots[P_ASC - object_diff].sign, get_sign_name((int)plots[P_ASC - object_diff].longitude / 30), get_sign_element((int)plots[P_ASC - object_diff].longitude / 30));
    wattroff(pad, COLOR_PAIR(15) | A_BOLD);

    row_pad += 2;

    const char *motivation = get_motivation((int)plots[P_ASC - object_diff].longitude / 30);
    
    wattron(pad, COLOR_PAIR(22) | A_BOLD);
    mvwprintw(pad, row_pad + 1, 6, _("The Core Motivation:"));
    wattroff(pad, COLOR_PAIR(22) | A_BOLD);

    row_pad += 1;

    wattron(pad, A_ITALIC);
    mvwprintw(pad, row_pad + 1, 8, "%s", motivation);
    wattroff(pad, A_ITALIC);

    row_pad += 2;

    int ruler = obter_regente_tradicional((int)plots[P_ASC - object_diff].longitude / 30 + 1);

    wattron(pad, COLOR_PAIR(28) | A_BOLD | A_REVERSE);
    mvwprintw(pad, row_pad + 2, 4, "• %s: %s (%s) ", _("Ruler of The Ascendant"), obter_glifo_planeta_por_id(ruler), obter_nome_planeta_por_id(ruler));
    wattroff(pad, COLOR_PAIR(28) | A_BOLD | A_REVERSE);

    row_pad += 2;

    wattron(pad, COLOR_PAIR(10) | A_DIM);
    mvwprintw(pad, row_pad + 1, 4, "───────────────────────────────────────────────────────────────────────────────────");
    wattroff(pad, COLOR_PAIR(10) | A_DIM);

    row_pad++;

    const char *planet_modifier = get_planet_motivation_modifier(ruler - 1);

    wattron(pad, A_ITALIC);

    


    int lines = print_text_multiline(pad, row_pad + 2, 6, 80, planet_modifier);


    row_pad += lines + 2;


        
    wattroff(pad, A_ITALIC);

    wattron(pad, COLOR_PAIR(21) | A_BOLD);
    mvwprintw(pad, row_pad + 2, 4, "• %s: %s (%s) %s  ", _("Sign of the Ruler"), plots[ruler - 1].sign, get_sign_name((int)plots[ruler - 1].longitude / 30), get_sign_element((int)plots[ruler - 1].longitude / 30));
    wattroff(pad, COLOR_PAIR(21) | A_BOLD);

    row_pad += 2;

    wattron(pad, COLOR_PAIR(10) | A_DIM);
    mvwprintw(pad, row_pad + 1, 4, "───────────────────────────────────────────────────────────────────────────────────");
    wattroff(pad, COLOR_PAIR(10) | A_DIM);
    
    row_pad += 2;

    wattron(pad, A_ITALIC);

    int ruler_sign = (int)plots[ruler - 1].longitude / 30;
    
    if (strcmp(get_sign_element_name(ruler_sign), _("Fire")) == 0) {
        mvwprintw(pad, row_pad + 1, 6, _("This drive is animated by assertive energy, dynamism, and leadership,"));
        mvwprintw(pad, row_pad + 2, 6, _("infusing the core motivation with passion and a vital quest for expansion."));
    }
    else if (strcmp(get_sign_element_name(ruler_sign), _("Earth")) == 0) {
        mvwprintw(pad, row_pad + 1, 6, _("This drive is anchored by pragmatism, enduring focus, and deliberation,"));
        mvwprintw(pad, row_pad + 2, 6, _("steering the core motivation toward tangible results and material stability."));
    }
    /* Corrigido para 'else if' para otimizar o fluxo de execução */
    else if (strcmp(get_sign_element_name(ruler_sign), _("Air")) == 0) {
        mvwprintw(pad, row_pad + 1, 6, _("This drive is channeled into intellectual versatility, logical synthesis,"));
        mvwprintw(pad, row_pad + 2, 6, _("and social exchange, occasionally spreading the core motivation across varied interests."));
    }
    else if (strcmp(get_sign_element_name(ruler_sign), _("Water")) == 0) {
        mvwprintw(pad, row_pad + 1, 6, _("This drive is shaped by emotional fluidity, intuition, and psychological depth,"));
        mvwprintw(pad, row_pad + 2, 6, _("rendering the core motivation highly adaptable to inner and outer currents."));
    }

    row_pad += 2;

    wattroff(pad, A_ITALIC);
    
    wattron(pad, COLOR_PAIR(27)| A_REVERSE | A_BOLD);
    mvwprintw(pad, row_pad + 3, 4, "• %s: %s ", _("House of the Ruler"), plots[ruler - 1].house);    
    wattroff(pad, COLOR_PAIR(27)| A_REVERSE | A_BOLD);

    row_pad += 3;

    wattron(pad, COLOR_PAIR(10) | A_DIM);
    mvwprintw(pad, row_pad + 1, 4, "───────────────────────────────────────────────────────────────────────────────────");
    wattroff(pad, COLOR_PAIR(10) | A_DIM);
   
    row_pad += 2;

    wattron(pad, COLOR_PAIR(22) | A_BOLD);
    mvwprintw(pad, row_pad + 1, 6, _("Areas of life where the Primary Motivation is applied or directed:"));
    wattroff(pad, COLOR_PAIR(22) | A_BOLD);

    row_pad++;

    int house_pos = romanToInt(plots[ruler - 1].house);
    const char *area_foco = get_house_modifier(house_pos);

    int casas_governadas[2] = {0, 0};
    int qtd_governadas = 0;

    for (int i = 1; i <= 12; i++) {
        if (house_rulers[i] == ruler) {
            casas_governadas[qtd_governadas] = i;
            qtd_governadas++;
            if (qtd_governadas >= 2) break; // Máximo de duas casas por planeta na astrologia tradicional
        }
    }

    char influence[1024] = "";

    if (qtd_governadas == 1) {
        // Caso do Sol, Lua, ou interceptações onde apenas uma casa é governada
        snprintf(influence, sizeof(influence),
            _("The native's primary drives are directed toward %s (House %d). "
              "The raw material and circumstances to fulfill this motivation will be "
              "drawn fundamentally from the affairs of %s (House %d)."),
            area_foco, house_pos, 
            get_house_modifier(casas_governadas[0]), casas_governadas[0]);
    } 
    else if (qtd_governadas == 2) {
        // Caso padrão dos planetas tradicionais (Mercúrio, Vênus, Marte, Júpiter, Saturno)
        snprintf(influence, sizeof(influence),
            _("The native's primary drives are directed toward %s (House %d). "
              "The fuel, tools, and background experiences for these matters are supplied "
              "by a dual matrix: the affairs of %s (House %d) and %s (House %d)."),
            area_foco, house_pos, 
            get_house_modifier(casas_governadas[0]), casas_governadas[0],
            get_house_modifier(casas_governadas[1]), casas_governadas[1]);
    }

    wattron(pad, A_ITALIC);
    lines = print_text_multiline(pad, row_pad + 2, 6, 85, influence);
    row_pad += lines + 1;
    wattroff(pad, A_ITALIC);

    
    mvwprintw(table_win, table_height - 1, 2, _(" Press ESC to return to chart | [↓↑] Scroll "));
    wrefresh(table_win);

    wrefresh(table_win);

    int offset_y = 0;
    int max_scroll_y = row_pad - max_linhas_dados + 2;
    if (max_scroll_y < 0) max_scroll_y = 0;

    // Vincula o teclado à PAD virtual
    keypad(pad, TRUE);
    nodelay(pad, FALSE);

    // Renderiza a primeira foto da PAD na tela
    prefresh(pad, offset_y + 1, 0, start_y + 3, start_x + 2, start_y + table_height - 3, start_x + table_width - 3);
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
        prefresh(pad, offset_y + 1, 0, start_y + 3, start_x + 2, start_y + table_height - 3, start_x + table_width - 3);        
    }
    
    // CLEAN UP: Desaloca todas as janelas do escopo e devolve o controle para a stdscr limpa
    delwin(pad);
    delwin(shadow_win);
    delwin(table_win);
    refresh();
}